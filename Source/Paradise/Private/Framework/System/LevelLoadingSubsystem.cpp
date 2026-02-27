// Copyright (C) Project Paradise. All Rights Reserved.


#include "Framework/System/LevelLoadingSubsystem.h"
#include "UI/Widgets/Loading/LoadingWidget.h" // 경로 확인 필요 (없으면 전방선언으로 대체하고 Cast)
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"

ULevelLoadingSubsystem::ULevelLoadingSubsystem()
{
	// 생성자 초기화
}

void ULevelLoadingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 맵 로드 완료 델리게이트 바인딩 (레벨 이동 감지용)
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULevelLoadingSubsystem::OnMapLoadComplete);

}

void ULevelLoadingSubsystem::Deinitialize()
{
	// 델리게이트 해제
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	// 진행 중인 타이머가 있다면 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProgressTimerHandle);
	}

	// 핸들 및 위젯 정리
	if (CurrentLoadHandle.IsValid())
	{
		CurrentLoadHandle->CancelHandle();
		CurrentLoadHandle.Reset();
	}

	CurrentLoadingWidget = nullptr;

	Super::Deinitialize();
}

#pragma region 외부 인터페이스
void ULevelLoadingSubsystem::StartLevelTransition(
	FName InTargetLevelName, 
	FName InLoadingMapName, 
	const TArray<TSoftObjectPtr<UObject>>& InAssetsToPreload, 
	TSoftObjectPtr<UTexture2D> InLoadingImage, 
	FText InStageName, 
	FText InStageDesc)
{
	if (InTargetLevelName.IsNone()) return;

	// 떠나기 전, 현재 월드의 이름을 '출발지'로 저장합니다.
	if (UWorld* World = GetWorld())
	{
		FString CurrentWorldName = World->GetMapName();
		CurrentWorldName.RemoveFromStart(World->StreamingLevelsPrefix);
		OriginLevelName = FName(*CurrentWorldName);
	}

	// 1. 상태 및 텍스트 캐싱
	TargetLevelName = InTargetLevelName;
	LoadingMapName = (InLoadingMapName.IsNone()) ? FName("L_Loading") : InLoadingMapName;
	PendingAssetsToLoad = InAssetsToPreload;
	PendingLoadingImage = InLoadingImage;

	PendingStageName = InStageName; 
	PendingStageDesc = InStageDesc; 

	bIsLoadingInProgress = true;
	// 이미지는 위젯 내부의 InitLoadingImage에서 스스로 판단합니다.

	UGameplayStatics::OpenLevel(this, LoadingMapName);
}

void ULevelLoadingSubsystem::SetLoadingWidgetClass(TSubclassOf<UUserWidget> NewLoadingWidgetClass)
{
	LoadingWidgetClass = NewLoadingWidgetClass;
}
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
void ULevelLoadingSubsystem::OnMapLoadComplete(UWorld* World)
{
	// 로딩 프로세스 중이 아니면 무시
	if (!bIsLoadingInProgress) return;

	// 현재 열린 맵의 이름을 확인 (접두사/경로 처리 주의)
	FString CurrentMapName = World->GetMapName();

	// PIE(에디터) 환경에서는 UEDPIE_0_ 같은 접두사가 붙을 수 있으므로 Contains로 확인하거나 RemovePrefix 처리
	CurrentMapName.RemoveFromStart(World->StreamingLevelsPrefix);

	UE_LOG(LogTemp, Log, TEXT("[LoadingSystem] 맵 로드 감지: %s"), *CurrentMapName);

	// 로딩 맵에 도착했는지 확인
	if (CurrentMapName.Contains(LoadingMapName.ToString()))
	{
		UE_LOG(LogTemp, Log, TEXT("[LoadingSystem] 로딩 맵 진입 성공. 비동기 로딩을 시작합니다."));
		BeginAsyncLoading();
	}
}

void ULevelLoadingSubsystem::BeginAsyncLoading()
{
	UWorld* World = GetWorld();
	if (!World || !LoadingWidgetClass) return;

	TotalElapsedTime = 0.0f;

	// 1. 위젯 생성 및 부착
	CurrentLoadingWidget = CreateWidget<ULoadingWidget>(World, LoadingWidgetClass);
	if (CurrentLoadingWidget)
	{
		CurrentLoadingWidget->AddToViewport(9999);

		CurrentLoadingWidget->InitLoadingImage(OriginLevelName, TargetLevelName, PendingLoadingImage);

		CurrentLoadingWidget->SetLoadingText(PendingStageName, PendingStageDesc);
	}

	// 비동기 로딩 요청
	if (PendingAssetsToLoad.Num() > 0)
	{
		TArray<FSoftObjectPath> AssetPaths;
		for (const auto& AssetPtr : PendingAssetsToLoad)
		{
			if (!AssetPtr.IsNull()) AssetPaths.Add(AssetPtr.ToSoftObjectPath());
		}
		if (AssetPaths.Num() > 0) CurrentLoadHandle = StreamableManager.RequestAsyncLoad(AssetPaths);
	}

	World->GetTimerManager().SetTimer(ProgressTimerHandle, this, &ULevelLoadingSubsystem::UpdateLoadingProgress, 0.05f, true);
}

void ULevelLoadingSubsystem::UpdateLoadingProgress()
{
	TotalElapsedTime += 0.05f;

	// 1. 시간 비율 (2초 기준)
	const float TimeRatio = FMath::Clamp(TotalElapsedTime / MinLoadingTime, 0.0f, 1.0f);
	// 2. 가짜 진행률 (0~70%)
	const float FakeProgress = TimeRatio * MaxFakePercent;

	// 3. 실제 로딩 비율
	float RealProgress = 1.0f;
	if (CurrentLoadHandle.IsValid()) RealProgress = CurrentLoadHandle->GetProgress();

	// 4. 최종 퍼센트: Fake(0~0.7) + (Real(0~1) * 0.3)
	float FinalDisplayPercent = FakeProgress + (RealProgress * (1.0f - MaxFakePercent));

	const bool bIsRealLoadingFinished = (!CurrentLoadHandle.IsValid() || CurrentLoadHandle->HasLoadCompleted());
	const bool bIsTimeFinished = (TimeRatio >= 1.0f);

	if (bIsRealLoadingFinished && bIsTimeFinished)
	{
		FinalDisplayPercent = 1.0f;
		if (CurrentLoadingWidget) CurrentLoadingWidget->SetLoadingPercent(1.0f);
		FinishLoading();
	}
	else
	{
		if (CurrentLoadingWidget) CurrentLoadingWidget->SetLoadingPercent(FinalDisplayPercent);
	}
}

void ULevelLoadingSubsystem::FinishLoading()
{
	UE_LOG(LogTemp, Log, TEXT("[LoadingSystem] 로딩 및 최소 대기 시간 종료. 최종 레벨로 이동합니다."));

	// 타이머 정지
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProgressTimerHandle);
	}

	// 핸들 해제
	if (CurrentLoadHandle.IsValid())
	{
		CurrentLoadHandle.Reset();
	}

	// 상태 플래그 해제
	bIsLoadingInProgress = false;

	// [중요] 위젯을 수동으로 지우지 않음!
	// OpenLevel이 호출되는 찰나에 배경이 비치는 것을 방지하기 위함.
	// 레벨이 바뀌면 위젯은 자동으로 파괴(GC)됩니다.
	CurrentLoadingWidget = nullptr;

	// 최종 레벨로 이동
	UGameplayStatics::OpenLevel(this, TargetLevelName);

}
#pragma endregion 내부 로직