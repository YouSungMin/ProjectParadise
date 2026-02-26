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

	UE_LOG(LogTemp, Log, TEXT("[LoadingSystem] 서브시스템 초기화 완료."));
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
void ULevelLoadingSubsystem::StartLevelTransition(FName InTargetLevelName, FName InLoadingMapName, const TArray<TSoftObjectPtr<UObject>>& InAssetsToPreload, TSoftObjectPtr<UTexture2D> InLoadingImage)
{
	if (InTargetLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadingSystem] 목표 레벨 이름이 유효하지 않습니다."));
		return;
	}

	// 1. 데이터 캐싱
	TargetLevelName = InTargetLevelName;

	// [수정] 만약 InLoadingMapName이 None이면 기본값 "L_Loading" 사용
	LoadingMapName = (InLoadingMapName.IsNone()) ? FName("L_Loading") : InLoadingMapName;

	PendingAssetsToLoad = InAssetsToPreload;
	PendingLoadingImage = InLoadingImage;

	bIsLoadingInProgress = true;

	UE_LOG(LogTemp, Log, TEXT("[LoadingSystem] 전이 시작: 현재 레벨 -> %s (Target: %s)"), *LoadingMapName.ToString(), *TargetLevelName.ToString());

	// 2. 로딩 맵(전이 맵)으로 이동
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
	if (!World) return;

	// 1. 데이터 초기화
	TotalElapsedTime = 0.0f;

	// 2. 로딩 위젯 생성 및 부착
	if (LoadingWidgetClass)
	{
		// 혹시 모를 이전 위젯 정리
		if (CurrentLoadingWidget)
		{
			CurrentLoadingWidget->RemoveFromParent();
			CurrentLoadingWidget = nullptr;
		}

		// 위젯 생성
		CurrentLoadingWidget = CreateWidget<ULoadingWidget>(World, LoadingWidgetClass);
		if (CurrentLoadingWidget)
		{
			CurrentLoadingWidget->AddToViewport(9999); // 최상위 Z-Order
			CurrentLoadingWidget->SetLoadingPercent(0.0f);

			// 캐싱해둔 커스텀 배경 이미지가 있다면 텍스처를 로드하여 UI에 적용합니다.
			if (!PendingLoadingImage.IsNull())
			{
				if (UTexture2D* LoadedTex = PendingLoadingImage.LoadSynchronous())
				{
					CurrentLoadingWidget->SetBackgroundImage(LoadedTex);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[LoadingSystem] LoadingWidgetClass가 설정되지 않았습니다! GameInstance Init을 확인하세요."));
	}

	// 3. 비동기 로딩 요청 (에셋 프리로딩)
	// Target Level 자체는 OpenLevel로 열지만, 그 전에 무거운 에셋들을 메모리에 올립니다.
	if (PendingAssetsToLoad.Num() > 0)
	{
		TArray<FSoftObjectPath> AssetPaths;
		for (const auto& AssetPtr : PendingAssetsToLoad)
		{
			if (!AssetPtr.IsNull())
			{
				AssetPaths.Add(AssetPtr.ToSoftObjectPath());
			}
		}

		if (AssetPaths.Num() > 0)
		{
			CurrentLoadHandle = StreamableManager.RequestAsyncLoad(AssetPaths);
		}
	}

	// 에셋이 없어도 로딩 바 연출을 위해 핸들 리셋
	if (!CurrentLoadHandle.IsValid())
	{
		CurrentLoadHandle.Reset();
	}

	// 4. 타이머 시작 (0.05초 간격)
	World->GetTimerManager().SetTimer(
		ProgressTimerHandle,
		this,
		&ULevelLoadingSubsystem::UpdateLoadingProgress,
		0.05f,
		true
	);
}

void ULevelLoadingSubsystem::UpdateLoadingProgress()
{
	// 1. 시간 누적 (0.05초)
	TotalElapsedTime += 0.05f;

	// 2. 시간 기반 진행률 (MinLoadingTime 기준)
	// 2초 동안 0 -> 1로 선형 보간
	const float TimeProgress = (MinLoadingTime > 0.0f) ? (TotalElapsedTime / MinLoadingTime) : 1.0f;

	// 3. 실제 에셋 로딩 진행률
	float RealProgress = 1.0f;
	if (CurrentLoadHandle.IsValid())
	{
		RealProgress = CurrentLoadHandle->GetProgress();
	}

	// 4. UI 갱신: (실제 로딩)과 (시간 흐름) 중 '더 느린 쪽'을 보여줌 -> 급발진 방지
	const float FinalDisplayPercent = FMath::Min(RealProgress, TimeProgress);

	if (CurrentLoadingWidget)
	{
		CurrentLoadingWidget->SetLoadingPercent(FinalDisplayPercent);
	}

	// 5. 완료 조건 체크
	// (실제 로딩 완료) AND (최소 시간 2초 경과)
	const bool bIsRealLoadingFinished = (!CurrentLoadHandle.IsValid() || CurrentLoadHandle->HasLoadCompleted());
	const bool bIsTimeFinished = (TimeProgress >= 1.0f);

	if (bIsRealLoadingFinished && bIsTimeFinished)
	{
		FinishLoading();
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