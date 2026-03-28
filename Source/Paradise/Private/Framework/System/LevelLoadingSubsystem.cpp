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

	CurrentPhase = ELoadingPhase::Appearing;

	if (UWorld* World = GetWorld())
	{
		CurrentLoadingWidget = CreateWidget<ULoadingWidget>(World, LoadingWidgetClass);
		if (CurrentLoadingWidget)
		{
			CurrentLoadingWidget->AddToViewport(99999);

			// ✅ Anim_Appear 재생(다음 프레임) 전에 배경 먼저 세팅
			CurrentLoadingWidget->InitLoadingImage(OriginLevelName, TargetLevelName, PendingLoadingImage);
			CurrentLoadingWidget->SetLoadingText(PendingStageName, PendingStageDesc);
		}
	}
}
void ULevelLoadingSubsystem::NotifyAppearFinished()
{
	// Anim_Appear 완료 → L_Loading으로 이동
	CurrentLoadingWidget = nullptr;
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

	// 로딩 맵에 도착했는지 확인
	if (CurrentMapName.Contains(LoadingMapName.ToString()))
	{
		CurrentPhase = ELoadingPhase::Loading;
		BeginAsyncLoading();
	}
	else if (CurrentMapName.Contains(TargetLevelName.ToString()))
	{
		CurrentPhase = ELoadingPhase::Disappearing;

		if (LoadingWidgetClass)
		{
			CurrentLoadingWidget = CreateWidget<ULoadingWidget>(World, LoadingWidgetClass);
			if (CurrentLoadingWidget)
			{
				CurrentLoadingWidget->AddToViewport(9999);
				CurrentLoadingWidget->InitAsCovered();

				// ✅ 배경 이미지/텍스트 세팅 추가
				CurrentLoadingWidget->InitLoadingImage(OriginLevelName, TargetLevelName, PendingLoadingImage);
				CurrentLoadingWidget->SetLoadingText(PendingStageName, PendingStageDesc);

				CurrentLoadingWidget->SetLoadingPercent(1.0f);

				World->GetTimerManager().SetTimerForNextTick(
					FTimerDelegate::CreateWeakLambda(CurrentLoadingWidget.Get(), [this]()
						{
							if (CurrentLoadingWidget)
							{
								CurrentLoadingWidget->PlayDisappearAnim();
							}
						})
				);
			}
		}
	}
}

void ULevelLoadingSubsystem::BeginAsyncLoading()
{
	UWorld* World = GetWorld();
	if (!World || !LoadingWidgetClass) return;

	// ✅ 이전 타이머 명시적 정리 후 초기화
	World->GetTimerManager().ClearTimer(ProgressTimerHandle);
	TotalElapsedTime = 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("[Loading] BeginAsyncLoading 시작 - TotalElapsedTime 초기화: %.2f"), TotalElapsedTime);

	// 1. 위젯 생성 및 부착
	CurrentLoadingWidget = CreateWidget<ULoadingWidget>(World, LoadingWidgetClass);
	if (CurrentLoadingWidget)
	{
		CurrentLoadingWidget->AddToViewport(9999);

		// ✅ Progress=1 유지 → 로딩 컨텐츠 완전히 표시
		CurrentLoadingWidget->InitAsCovered();

		// ✅ 배경/텍스트 세팅
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

	World->GetTimerManager().SetTimerForNextTick([this, World]()
		{
			World->GetTimerManager().SetTimer(
				ProgressTimerHandle, this,
				&ULevelLoadingSubsystem::UpdateLoadingProgress, 0.05f, true);
		});
}

void ULevelLoadingSubsystem::UpdateLoadingProgress()
{
	TotalElapsedTime += 0.05f;

	// 1. 시간 비율 (2초 기준)
	const float TimeRatio = FMath::Clamp(TotalElapsedTime / MinLoadingTime, 0.0f, 1.0f);
	// 2. 가짜 진행률 (0~70%)
	const float FakeProgress = TimeRatio * MaxFakePercent;

	float FinalDisplayPercent = FakeProgress;

	const bool bIsTimeFinished = (TimeRatio >= 1.0f);

	if (bIsTimeFinished)
	{
		// 2. 2초 경과 후
		if (CurrentLoadHandle.IsValid())
		{
			// 에셋 있으면 70%~100% 사이를 실제 로딩 비율로 채움
			float RealProgress = CurrentLoadHandle->GetProgress();
			FinalDisplayPercent = MaxFakePercent + (RealProgress * (1.0f - MaxFakePercent));

			if (CurrentLoadHandle->HasLoadCompleted())
			{
				// 에셋 로딩 완료 → 100%
				if (CurrentLoadingWidget) CurrentLoadingWidget->SetLoadingPercent(1.0f);
				FinishLoading();
				return;
			}
		}
		else
		{
			// 에셋 없으면 즉시 100%
			if (CurrentLoadingWidget) CurrentLoadingWidget->SetLoadingPercent(1.0f);
			FinishLoading();
			return;
		}
	}
	if (CurrentLoadingWidget) CurrentLoadingWidget->SetLoadingPercent(FinalDisplayPercent);
}

void ULevelLoadingSubsystem::FinishLoading()
{
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
	CurrentLoadingWidget = nullptr;

	UGameplayStatics::OpenLevel(this, TargetLevelName);
}

void ULevelLoadingSubsystem::NotifyDisappearFinished()
{
	// Anim_Disappear 완료 → 위젯 제거, 끝
	if (CurrentLoadingWidget)
	{
		CurrentLoadingWidget->RemoveFromParent();
		CurrentLoadingWidget = nullptr;
	}
	bIsLoadingInProgress = false;
	CurrentPhase = ELoadingPhase::None;
}
#pragma endregion 내부 로직

void ULevelLoadingSubsystem::ExecuteFinalTransition()
{
	CurrentLoadingWidget = nullptr;
	UGameplayStatics::OpenLevel(this, TargetLevelName);
}
