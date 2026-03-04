// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Gacha/ParadiseGachaBoxActor.h"
#include "Actors/Gacha/ParadiseGachaItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "NiagaraFunctionLibrary.h"

#pragma region 초기화 및 생명주기
AParadiseGachaBoxActor::AParadiseGachaBoxActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	RootComponent = BoxMesh;
}

void AParadiseGachaBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SequencePlayer)
	{
		SequencePlayer->OnFinished.RemoveAll(this);
		SequencePlayer = nullptr;
	}
	SequenceActor = nullptr;

	Super::EndPlay(EndPlayReason);
}
#pragma endregion 초기화 및 생명주기

#pragma region 연출 제어 로직 구현
void AParadiseGachaBoxActor::PlayGachaSequence(const TArray<FGachaResult>& InResults)
{
	if (InResults.IsEmpty()) return;

	CachedResults = InResults;
	CachedHighestRarity = GetHighestRarity(CachedResults);

	UpdateBoxMaterialByRarity(CachedHighestRarity);

	ULevelSequence* TargetSequence = (CachedResults.Num() == 1) ? SingleGachaSequence : MultiGachaSequence;

	if (!TargetSequence)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaBox] 시퀀스 에셋 누락! 즉시 스킵 처리합니다."));
		SkipGachaSequence();
		return;
	}

	if (SequencePlayer)
	{
		SequencePlayer->Stop();
		SequencePlayer->OnFinished.RemoveAll(this);
		SequencePlayer = nullptr;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.bPauseAtEnd = true;

	ALevelSequenceActor* OutSequenceActor = nullptr;

	SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(),
		TargetSequence,
		PlaybackSettings,
		OutSequenceActor // <-- 여기에 임시 포인터를 넣습니다.
	);

	SequenceActor = OutSequenceActor;

	if (SequencePlayer)
	{
		SequencePlayer->OnFinished.AddDynamic(this, &AParadiseGachaBoxActor::HandleSequenceFinished);
		SequencePlayer->Play();
	}
}

void AParadiseGachaBoxActor::SetGachaPlaySpeed(float SpeedMultiplier)
{
	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		SequencePlayer->SetPlayRate(SpeedMultiplier);
	}
}

void AParadiseGachaBoxActor::SkipGachaSequence()
{
	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		SequencePlayer->GoToEndAndStop();
	}
	else
	{
		HandleSequenceFinished();
	}
}
#pragma endregion 연출 제어 로직 구현

#pragma region 내부 유틸리티 및 이펙트 로직
EItemRarity AParadiseGachaBoxActor::GetHighestRarity(const TArray<FGachaResult>& Results) const
{
	EItemRarity Highest = EItemRarity::Common;
	for (const FGachaResult& Result : Results)
	{
		// 열거형(Enum) 값의 크기 비교 (Common < Uncommon < Rare < Epic < Legend 순서라고 가정)
		if (Result.PulledRarity > Highest)
		{
			Highest = Result.PulledRarity;
		}
	}
	return Highest;
}

void AParadiseGachaBoxActor::UpdateBoxMaterialByRarity(EItemRarity HighestRarity)
{
	if (TObjectPtr<UMaterialInstance>* MatPtr = BoxMaterialsByRarity.Find(HighestRarity))
	{
		if (MatPtr && *MatPtr)
		{
			BoxMesh->SetMaterial(0, *MatPtr);
		}
	}
}

void AParadiseGachaBoxActor::SpawnClimaxEffect()
{
	if (TObjectPtr<UNiagaraSystem>* FxPtr = ClimaxEffectsByRarity.Find(CachedHighestRarity))
	{
		if (FxPtr && *FxPtr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), *FxPtr, GetActorLocation(), GetActorRotation());
		}
	}
}

void AParadiseGachaBoxActor::HandleSequenceFinished()
{
	if (OnGachaResultScreenRequested.IsBound())
	{
		//  최종 UI 결과창에 사용자가 만든 완벽한 데이터 구조체를 통째로 전달
		OnGachaResultScreenRequested.Broadcast(CachedResults);
	}
}
void AParadiseGachaBoxActor::EruptGachaItems()
{
	// 1. 이펙트 터뜨리기 (기존 로직)
	SpawnClimaxEffect();

	if (!ItemActorClass || CachedResults.IsEmpty()) return;

	// 2. 1회 소환용 사출 로직 (일단 위로 퐁! 하고 던짐)
	FVector SpawnLoc = GetActorLocation() + FVector(0, 0, 50.0f); // 상자 살짝 위
	FRotator SpawnRot = FRotator::ZeroRotator;

	// 구슬 스폰
	AParadiseGachaItemActor* SpawnedItem = GetWorld()->SpawnActor<AParadiseGachaItemActor>(ItemActorClass, SpawnLoc, SpawnRot);

	if (SpawnedItem)
	{
		// 유저님이 짠 완벽한 데이터 주입 함수 호출! (머티리얼은 일단 임시로 nullptr 처리)
		SpawnedItem->InitializeItemData(CachedResults[0], nullptr, nullptr);

		// 상자 앞쪽 바닥으로 날아가도록 목표 지점 설정
		FVector TargetLoc = GetActorLocation() + GetActorForwardVector() * 150.0f;
		TargetLoc.Z = GetActorLocation().Z; // 바닥 높이

		// 1초 동안 높이 200만큼 튀어오르며 날아감
		SpawnedItem->LaunchToTarget(TargetLoc, 1.0f, 200.0f);
	}
}
#pragma endregion 내부 유틸리티 및 이펙트 로직

