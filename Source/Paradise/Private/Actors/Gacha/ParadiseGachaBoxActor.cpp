// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Gacha/ParadiseGachaBoxActor.h"
#include "Actors/Gacha/ParadiseGachaItemActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/PlayerController.h"

#pragma region 초기화 및 생명주기
AParadiseGachaBoxActor::AParadiseGachaBoxActor()
{
	// 발광 보간을 위해 Tick 활성화
	PrimaryActorTick.bCanEverTick = true;

	BoxMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BoxMesh"));
	RootComponent = BoxMesh;

}

void AParadiseGachaBoxActor::BeginPlay()
{
	Super::BeginPlay();

	if (!BoxMesh) return;

	BoxDynamicMat = BoxMesh->CreateAndSetMaterialInstanceDynamic(0);

	// PC/에디터: 마우스 클릭 감지 (OnClicked = 마우스 버튼을 눌렀다 뗄 때 발동)
	BoxMesh->OnClicked.AddDynamic(this, &AParadiseGachaBoxActor::HandleBoxClicked);

	// 모바일: 터치 시작/종료 감지
	// OnInputTouchBegin → 손가락 닿는 순간 (꾹 누름 배속 시작)
	// OnInputTouchEnd   → 손가락 떼는 순간 (탭 감지 + 배속 복귀)
	BoxMesh->OnInputTouchBegin.AddDynamic(this, &AParadiseGachaBoxActor::HandleBoxTouchBegin);
	BoxMesh->OnInputTouchEnd.AddDynamic(this, &AParadiseGachaBoxActor::HandleBoxTouchEnd);
}

void AParadiseGachaBoxActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickPressUpdate();
	TickGlowUpdate(DeltaTime);
}

void AParadiseGachaBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (FTimerHandle& Handle : SpawnTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	SpawnTimerHandles.Empty();

	StopCurrentSequence();

	Super::EndPlay(EndPlayReason);
}
#pragma endregion 초기화 및 생명주기

#pragma region 외부 인터페이스 구현
void AParadiseGachaBoxActor::PlayGachaSequence(const TArray<FGachaResult>& InResults)
{
	if (InResults.IsEmpty()) return;

	CachedResults = InResults;
	CachedHighestRarity = GetHighestRarity(CachedResults);
	bIsOpening = false;

	// ★ 머티리얼 교체는 상자가 열리는 순간(ApplyClimaxVisual)에만 합니다.

	if (!IntroSequence)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaBox] IntroSequence 누락! Idle로 바로 진입합니다."));
		CurrentStep = EGachaSequenceStep::Intro;
		OnSequenceFinished();
		return;
	}

	PlaySequenceInternal(IntroSequence, EGachaSequenceStep::Intro, false);
}

void AParadiseGachaBoxActor::SkipGachaSequence()
{
	for (FTimerHandle& Handle : SpawnTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	SpawnTimerHandles.Empty();

	StopCurrentSequence();

	// Open 단계로 강제 전환 → 결과창 바로 표시
	CurrentStep = EGachaSequenceStep::Open;
	OnSequenceFinished();
}

void AParadiseGachaBoxActor::SetGachaPlaySpeed(float SpeedMultiplier)
{
	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		SequencePlayer->SetPlayRate(SpeedMultiplier);
	}
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 시퀀스 로직 구현
void AParadiseGachaBoxActor::PlaySequenceInternal(
	ULevelSequence* Sequence, EGachaSequenceStep Step, bool bLoop)
{
	if (!Sequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [GachaBox] PlaySequenceInternal: Sequence가 nullptr입니다."));
		CurrentStep = Step;
		OnSequenceFinished();
		return;
	}

	StopCurrentSequence();
	CurrentStep = Step;

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bPauseAtEnd = !bLoop;
	Settings.LoopCount.Value = bLoop ? -1 : 0; // -1 = 무한 루프

	ALevelSequenceActor* OutActor = nullptr;
	SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), Sequence, Settings, OutActor);
	SequenceActor = OutActor;

	if (!SequencePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaBox] LevelSequencePlayer 생성 실패!"));
		OnSequenceFinished();
		return;
	}

	if (!bLoop)
	{
		SequencePlayer->OnFinished.AddDynamic(this, &AParadiseGachaBoxActor::OnSequenceFinished);
	}

	SequencePlayer->Play();
}

void AParadiseGachaBoxActor::StopCurrentSequence()
{
	if (SequencePlayer)
	{
		SequencePlayer->Stop();
		SequencePlayer->OnFinished.RemoveAll(this);
		SequencePlayer = nullptr;
	}
	SequenceActor = nullptr;
}

void AParadiseGachaBoxActor::OnSequenceFinished()
{
	// CurrentStep 을 보고 다음 단계로 전환합니다. 
	switch (CurrentStep)
	{
	case EGachaSequenceStep::Intro:
		// 낙하+착지 완료 → Idle 루프 시작
		if (!IdleShakeSequence)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [GachaBox] IdleShakeSequence 누락! 터치 대기 상태로 진입합니다."));
			return;
		}
		PlaySequenceInternal(IdleShakeSequence, EGachaSequenceStep::Idle, true);
		break;

	case EGachaSequenceStep::Open:
		// 격렬+열림 완료 → 결과창 요청
		if (OnGachaResultScreenRequested.IsBound())
		{
			OnGachaResultScreenRequested.Broadcast(CachedResults);
		}
		break;

	case EGachaSequenceStep::Idle:
	case EGachaSequenceStep::None:
	default:
		break;
	}
}

EItemRarity AParadiseGachaBoxActor::GetHighestRarity(const TArray<FGachaResult>& Results) const
{
	EItemRarity Highest = EItemRarity::Common;
	for (const FGachaResult& Result : Results)
	{
		if (Result.PulledRarity > Highest)
		{
			Highest = Result.PulledRarity;
		}
	}
	return Highest;
}

void AParadiseGachaBoxActor::ApplyClimaxVisual()
{
	// 1. 등급 머티리얼로 교체
	if (TObjectPtr<UMaterialInstance>* MatPtr = BoxMaterialsByRarity.Find(CachedHighestRarity))
	{
		if (MatPtr && *MatPtr && BoxMesh)
		{
			BoxMesh->SetMaterial(0, *MatPtr);
			// 교체 후 다이나믹 인스턴스 갱신 (발광 파라미터 계속 제어)
			BoxDynamicMat = BoxMesh->CreateAndSetMaterialInstanceDynamic(0);
		}
	}

	// 2. 폭발 이펙트 스폰
	if (TObjectPtr<UNiagaraSystem>* FxPtr = ClimaxEffectsByRarity.Find(CachedHighestRarity))
	{
		if (FxPtr && *FxPtr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), *FxPtr, GetActorLocation(), GetActorRotation());
		}
	}
}

void AParadiseGachaBoxActor::EruptGachaItems()
{
	ApplyClimaxVisual();

	if (!ItemActorClass || CachedResults.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaBox] ItemActorClass 또는 CachedResults 없음!"));
		return;
	}

	const int32 ItemCount = CachedResults.Num();

	// 타이머 핸들 배열 미리 확보 (메모리 재할당 방지)
	SpawnTimerHandles.Reset(ItemCount);
	SpawnTimerHandles.SetNum(ItemCount);

	for (int32 i = 0; i < ItemCount; ++i)
	{
		// 1연차: 딜레이 없이 즉시 / 10연차: MultiSpawnInterval 간격으로 슝슝슝
		const float Delay = (ItemCount > 1) ? (i * MultiSpawnInterval) : 0.0f;

		if (FMath::IsNearlyZero(Delay))
		{
			SpawnSingleItem(i);
		}
		else
		{
			GetWorldTimerManager().SetTimer(
				SpawnTimerHandles[i],
				FTimerDelegate::CreateUObject(this, &AParadiseGachaBoxActor::SpawnSingleItem, i),
				Delay,
				false
			);
		}
	}
}

void AParadiseGachaBoxActor::SpawnSingleItem(int32 Index)
{
	if (!CachedResults.IsValidIndex(Index)) return;

	const FVector BoxLoc = GetActorLocation();
	const FVector SpawnLoc = BoxLoc + FVector(0.0f, 0.0f, 50.0f);
	const int32   ItemCount = CachedResults.Num();

	AParadiseGachaItemActor* SpawnedItem = GetWorld()->SpawnActor<AParadiseGachaItemActor>(
		ItemActorClass, SpawnLoc, FRotator::ZeroRotator);

	if (!SpawnedItem) return;

	// 등급에 맞는 실루엣 머티리얼 조회
	UMaterialInstance* TargetSilhouetteMat = nullptr;
	if (TObjectPtr<UMaterialInstance>* MatPtr = SilhouetteMaterialsByRarity.Find(CachedResults[Index].PulledRarity))
	{
		TargetSilhouetteMat = *MatPtr;
	}

	SpawnedItem->InitializeItemData(CachedResults[Index], TargetSilhouetteMat, nullptr);

	FVector TargetLoc = BoxLoc;

	if (ItemCount == 1)
	{
		TargetLoc += GetActorForwardVector() * 200.0f;
	}
	else
	{
		const float AngleDeg = (360.0f / static_cast<float>(ItemCount)) * static_cast<float>(Index);
		TargetLoc.X += FMath::Cos(FMath::DegreesToRadians(AngleDeg)) * EruptRadius;
		TargetLoc.Y += FMath::Sin(FMath::DegreesToRadians(AngleDeg)) * EruptRadius;
	}

	TargetLoc.Z = BoxLoc.Z;

	const float FlightTime = FMath::RandRange(FlightTimeMin, FlightTimeMax);
	SpawnedItem->LaunchToTarget(TargetLoc, FlightTime, EruptArcHeight);
}
#pragma endregion 내부 시퀀스 로직 구현

#pragma region 내부 터치 로직 구현
void AParadiseGachaBoxActor::HandleBoxClicked(UPrimitiveComponent* TouchedComp, FKey ButtonPressed)
{
	ProcessTouchInput();
}

void AParadiseGachaBoxActor::HandleBoxTouchBegin(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComp)
{
	// 모바일 손가락 닿는 순간 → 꾹 누름 시작
	bIsMobilePressing = true;
}

void AParadiseGachaBoxActor::HandleBoxTouchEnd(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComp)
{
	// 모바일 손가락 떼는 순간 → 꾹 누름 해제 + 탭 감지
	bIsMobilePressing = false;
	ProcessTouchInput();
}

void AParadiseGachaBoxActor::ProcessTouchInput()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bDoubleTap = (LastTouchTime > 0.0f) && ((Now - LastTouchTime) <= DoubleTapThreshold);
	LastTouchTime = Now;

	if (bDoubleTap)
	{
		ProcessDoubleTap();
	}
	else
	{
		ProcessSingleTap();
	}
}

void AParadiseGachaBoxActor::ProcessSingleTap()
{
	// Open 시퀀스가 이미 재생 중이면 중복 실행 방지
	if (bIsOpening) return;
	bIsOpening = true;

	StopCurrentSequence();

	const bool bIsMulti = (CachedResults.Num() > 1);
	ULevelSequence* TargetSequence = bIsMulti ? OpenMultiSequence : OpenSingleSequence;

	if (!TargetSequence)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaBox] Open 시퀀스 누락! 스킵 처리합니다."));
		SkipGachaSequence();
		return;
	}

	// Open 시퀀스 재생 (이벤트 트랙에서 EruptGachaItems 호출, 완료 시 OnSequenceFinished → Open 분기)
	PlaySequenceInternal(TargetSequence, EGachaSequenceStep::Open, false);
}

void AParadiseGachaBoxActor::ProcessDoubleTap()
{
	SkipGachaSequence();
}

void AParadiseGachaBoxActor::TickPressUpdate()
{
	// 모바일: bIsMobilePressing 플래그로 판단
	// PC/에디터: PlayerController 에서 왼쪽 마우스 버튼 누름 여부 폴링
	bool bNewPressing = bIsMobilePressing;

	if (!bNewPressing)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			bNewPressing = PC->IsInputKeyDown(EKeys::LeftMouseButton);
		}
	}

	// 상태 변화가 있을 때만 배속 적용 (매 Tick SetPlayRate 호출 방지)
	if (bNewPressing != bIsPressing)
	{
		bIsPressing = bNewPressing;
		SetGachaPlaySpeed(bIsPressing ? PressPlayRate : 1.0f);
	}
}

void AParadiseGachaBoxActor::TickGlowUpdate(float DeltaTime)
{
	if (!BoxDynamicMat) return;

	const float Target = bIsPressing ? 1.0f : 0.0f;
	const float Speed = bIsPressing ? GlowIncreaseSpeed : GlowDecreaseSpeed;

	ShakeIntensity = FMath::FInterpTo(ShakeIntensity, Target, DeltaTime, Speed);

	BoxDynamicMat->SetScalarParameterValue(GlowIntensityParamName, ShakeIntensity * MaxGlowIntensity);
}
#pragma endregion 내부 터치 로직 구현

