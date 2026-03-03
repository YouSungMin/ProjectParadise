// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Gacha/ParadiseGachaBoxActor.h"
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
#pragma endregion 내부 유틸리티 및 이펙트 로직

