// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Gacha/ParadiseGachaBoxActor.h"
#include "Actors/Gacha/ParadiseGachaItemActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Assets/ParadiseFXAudioData.h"
#include "Components/AudioComponent.h"

#pragma region 초기화 및 생명주기
AParadiseGachaBoxActor::AParadiseGachaBoxActor()
{
	// 발광 보간을 위해 Tick 활성화
	PrimaryActorTick.bCanEverTick = true;

	BoxMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BoxMesh"));
	RootComponent = BoxMesh;

	// 2. 바닥 조명 설정
	AuraLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AuraLight"));
	AuraLight->SetupAttachment(RootComponent);
	AuraLight->SetIntensity(0.0f); // 기본 꺼짐 상태
	AuraLight->SetAttenuationRadius(500.0f); // 부드러운 감쇠 반경
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

	TickGlowUpdate(DeltaTime);
}

void AParadiseGachaBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (FTimerHandle& Handle : SpawnTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	SpawnTimerHandles.Empty();

	GetWorldTimerManager().ClearTimer(ResultDelayTimerHandle);
	StopCurrentSequence();

	Super::EndPlay(EndPlayReason);
}
#pragma endregion 초기화 및 생명주기

#pragma region 외부 인터페이스 구현
void AParadiseGachaBoxActor::PlayGachaSequence(const TArray<FGachaResult>& InResults)
{
	// 상자를 화면에 다시 보이게 하고 틱/충돌을 켭니다 (재사용)
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (InResults.IsEmpty()) return;

	CachedResults = InResults;
	CachedHighestRarity = GetHighestRarity(CachedResults);
	bIsOpening = false;
	RevealedItemCount = 0;
	TotalItemCount = InResults.Num();
	SpawnedItems.Empty();

	// 새로운 뽑기를 시작할 때 사운드 상태 초기화
	bHasPlayedOrbDropSound = false;
	if (CurrentRevealSoundComp && CurrentRevealSoundComp->IsPlaying())
	{
		CurrentRevealSoundComp->Stop();
	}

	// 시퀀스 재생과 동시에 타이머가 돌아가며, BoxDropSoundDelay 초 뒤에 정확히 쿵! 소리를 냅니다.
	if (BoxDropSoundDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			BoxDropSoundTimerHandle,
			this,
			&AParadiseGachaBoxActor::PlayBoxDropSound,
			BoxDropSoundDelay,
			false
		);
	}
	else
	{
		// 딜레이가 0이면 즉시 재생
		PlayBoxDropSound();
	}

	// 시퀀스 시작 전 라이트 및 발광 색상을 미리 초기화 
	if (FLinearColor* RarityColor = GlowColorsByRarity.Find(CachedHighestRarity))
	{
		if (AuraLight)
		{
			AuraLight->SetIntensity(0.0f);
			AuraLight->SetLightColor(*RarityColor);
		}

		if (BoxDynamicMat)
		{
			BoxDynamicMat->SetVectorParameterValue(GlowColorParamName, *RarityColor);
		}
	}

	if (!IntroSequence)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaBox] IntroSequence 누락! Idle로 바로 진입합니다."));
		CurrentStep = EGachaSequenceStep::Intro;
		OnSequenceFinished();
		return;
	}

	PlaySequenceInternal(IntroSequence, EGachaSequenceStep::Intro, false);
}

void AParadiseGachaBoxActor::PlayBoxDropSound()
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_GachaBoxDrop)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_GachaBoxDrop);
		}
	}
}

void AParadiseGachaBoxActor::SkipGachaSequence()
{
	for (FTimerHandle& Handle : SpawnTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	SpawnTimerHandles.Empty();

	GetWorldTimerManager().ClearTimer(ResultDelayTimerHandle);
	StopCurrentSequence();

	// 딜레이 없이 즉시 결과창
	ShowResultScreen();
}

void AParadiseGachaBoxActor::ResetState()
{
	// 1. 타이머 전부 취소
	for (FTimerHandle& Handle : SpawnTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	SpawnTimerHandles.Empty();
	GetWorldTimerManager().ClearTimer(ResultDelayTimerHandle);

	GetWorldTimerManager().ClearTimer(BoxDropSoundTimerHandle);

	// 2. 시퀀스 정지
	StopCurrentSequence();

	// 3. 구슬 전부 제거
	for (TObjectPtr<AParadiseGachaItemActor>& Item : SpawnedItems)
	{
		if (IsValid(Item)) Item->Destroy();
	}
	SpawnedItems.Empty();

	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 4. 내부 상태 초기화 (박스 자신은 Destroy 하지 않음)
	CachedResults.Empty();
	bIsOpening = false;
	RevealedItemCount = 0;
	TotalItemCount = 0;
	LastTouchTime = -1.0f;
	ShakeIntensity = 0.0f;
	CurrentStep = EGachaSequenceStep::None;

	if (AuraLight) AuraLight->SetIntensity(0.0f);
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
		SequencePlayer->Play();
	}
	else
	{
		SequencePlayer->PlayLooping(-1);
	}
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
		// Intro 종료 시 바닥 라이트 켜기 (은은한 효과 시작)
		if (AuraLight)
		{
			AuraLight->SetIntensity(IdleLightIntensity);
		}
		if (IdleShakeSequence)
		{
			PlaySequenceInternal(IdleShakeSequence, EGachaSequenceStep::Idle, true);
		}
		break;
	case EGachaSequenceStep::Open:
		// Open 시퀀스 종료 — 구슬들이 착지 후 터치 대기 중
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
	// 오픈 순간 포인트 라이트 끄기 (클라이맥스 이펙트에 집중)
	if (AuraLight)
	{
		AuraLight->SetIntensity(0.0f);
	}

	if (TObjectPtr<UNiagaraSystem>* FxPtr = ClimaxEffectsByRarity.Find(CachedHighestRarity))
	{
		if (FxPtr && *FxPtr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), *FxPtr, GetActorLocation(), GetActorRotation());
		}
	}
}

void AParadiseGachaBoxActor::EruptGachaItems()
{
	ApplyClimaxVisual();

	// 이미 이 함수 자체가 뚜껑 열릴 때 호출되도록 세팅되어 있으므로 타이밍이 완벽합니다!
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData)
		{
			// 1. 상자 뚜껑 덜컥! 열리는 소리
			if (GI->GlobalAudioData->SFX_GachaBoxTouchOpen)
			{
				UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_GachaBoxTouchOpen);
			}

			// 2. 구슬이 하늘로 슝~ 하고 솟구치는 소리 (10연차든 1연차든 여기서 딱 1번만 재생됨!)
			if (GI->GlobalAudioData->SFX_GachaOrbDrop)
			{
				UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_GachaOrbDrop);
			}
		}
	}

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
	// 착지 완료 콜백 바인딩
	SpawnedItem->OnItemLanded.AddDynamic(this, &AParadiseGachaBoxActor::OnItemLandedCallback);
	// 리빌 완료 콜백 바인딩
	SpawnedItem->OnItemRevealed.AddDynamic(this, &AParadiseGachaBoxActor::OnItemRevealedCallback);

	// 구슬 목록에 추가 (배속 전파용)
	SpawnedItems.Add(SpawnedItem);

	// 목표 위치 계산
	FVector TargetLoc = BoxLoc;
	if (ItemCount == 1)
	{
		// 박스의 로컬 좌표계 기준으로 착지 위치 계산
		// SinglePullLandingOffset = (0,0,0)이면 박스 정중앙 바닥에 착지
		TargetLoc += GetActorForwardVector() * SinglePullLandingOffset.X
					+ GetActorRightVector() * SinglePullLandingOffset.Y;
		TargetLoc.Z += SinglePullLandingOffset.Z + OrbLandingZOffset;

		// 1연차는 높은 아크로 하늘에서 뚝 떨어지는 연출
		SpawnedItem->LaunchToTarget(TargetLoc, FlightTimeMax, SinglePullArcHeight);
		return; // 아래 공통 LaunchToTarget 호출 건너뜀
	}
	else
	{
		const float AngleDeg = (360.0f / static_cast<float>(ItemCount)) * static_cast<float>(Index);
		TargetLoc.X += FMath::Cos(FMath::DegreesToRadians(AngleDeg)) * EruptRadius;
		TargetLoc.Y += FMath::Sin(FMath::DegreesToRadians(AngleDeg)) * EruptRadius;
	}

	TargetLoc.Z = BoxLoc.Z + OrbLandingZOffset;

	const float FlightTime = FMath::RandRange(FlightTimeMin, FlightTimeMax);
	SpawnedItem->LaunchToTarget(TargetLoc, FlightTime, EruptArcHeight);
}
#pragma endregion 내부 시퀀스 로직 구현

#pragma region 내부 리빌 카운트 로직 구현
void AParadiseGachaBoxActor::OnItemLandedCallback()
{
	++LandedItemCount;

	// 모든 구슬이 착지 완료되면 일괄 터치 활성화
	if (LandedItemCount >= TotalItemCount)
	{
		for (TObjectPtr<AParadiseGachaItemActor>& Item : SpawnedItems)
		{
			if (IsValid(Item)) Item->EnableTouch();
		}
	}
}
void AParadiseGachaBoxActor::OnItemRevealedCallback(const FGachaResult& ItemData)
{
	++RevealedItemCount;

	/** @section ⭐ 사운드: 구슬 리빌음 (오버랩 방지) */
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_GachaOrbReveal)
		{
			// 기존에 재생 중인 리빌 소리가 있다면 강제 종료 (따다닥! 눌렀을 때 소리 안 겹치게)
			if (CurrentRevealSoundComp && CurrentRevealSoundComp->IsPlaying())
			{
				CurrentRevealSoundComp->Stop();
			}
			// 새로운 리빌 사운드 시작 및 컴포넌트 갱신
			CurrentRevealSoundComp = UGameplayStatics::SpawnSound2D(this, GI->GlobalAudioData->SFX_GachaOrbReveal);
		}
	}

	if (OnSingleCharacterRevealed.IsBound())
	{
		OnSingleCharacterRevealed.Broadcast(ItemData);
	}

	// 모든 구슬 리빌 완료 → ResultDelaySeconds 후 결과창
	if (RevealedItemCount >= TotalItemCount)
	{
		GetWorldTimerManager().SetTimer(
			ResultDelayTimerHandle,
			this,
			&AParadiseGachaBoxActor::ShowResultScreen,
			ResultDelaySeconds,
			false);
	}
}

void AParadiseGachaBoxActor::ShowResultScreen()
{
	if (OnGachaResultScreenRequested.IsBound())
	{
		OnGachaResultScreenRequested.Broadcast(CachedResults);
	}
}
#pragma endregion 내부 리빌 카운트 로직 구현

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
	UE_LOG(LogTemp, Error, TEXT("👆 [GachaBox] 상자 터치 성공! Open 시퀀스로 넘어갑니다!"));

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

void AParadiseGachaBoxActor::TickGlowUpdate(float DeltaTime)
{
	if (!BoxDynamicMat) return;

	const float Target = bIsPressing ? 1.0f : 0.0f;
	const float Speed = bIsPressing ? GlowIncreaseSpeed : GlowDecreaseSpeed;

	ShakeIntensity = FMath::FInterpTo(ShakeIntensity, Target, DeltaTime, Speed);

	BoxDynamicMat->SetScalarParameterValue(GlowIntensityParamName, ShakeIntensity * MaxGlowIntensity);
}
#pragma endregion 내부 터치 로직 구현

