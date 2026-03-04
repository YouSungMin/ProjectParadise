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
	// 1. 최고 등급 이펙트 터뜨리기 (상자 열릴 때 번쩍!)
	SpawnClimaxEffect();

	if (!ItemActorClass || CachedResults.IsEmpty()) return;

	int32 ItemCount = CachedResults.Num();
	FVector BoxLoc = GetActorLocation();

	// 2. 배열에 들어있는 개수만큼(1개 or 10개) 구슬을 생성해서 던집니다.
	for (int32 i = 0; i < ItemCount; ++i)
	{
		// 상자 바로 위에서 스폰
		FVector SpawnLoc = BoxLoc + FVector(0, 0, 50.0f);
		AParadiseGachaItemActor* SpawnedItem = GetWorld()->SpawnActor<AParadiseGachaItemActor>(ItemActorClass, SpawnLoc, FRotator::ZeroRotator);

		if (SpawnedItem)
		{
			// 1. 방금 뽑힌 이 구슬의 등급(Rarity)에 맞는 실루엣 머티리얼을 맵에서 찾습니다.
			UMaterialInstance* TargetSilhouetteMat = nullptr;
			if (TObjectPtr<UMaterialInstance>* MatPtr = SilhouetteMaterialsByRarity.Find(CachedResults[i].PulledRarity))
			{
				TargetSilhouetteMat = *MatPtr;
			}

			// ★ 핵심: 찾은 실루엣 머티리얼을 구슬에게 주입! (진짜 캐릭터 머티리얼은 일단 nullptr)
			SpawnedItem->InitializeItemData(CachedResults[i], TargetSilhouetteMat, nullptr);

			FVector TargetLoc = BoxLoc;

			if (ItemCount == 1)
			{
				// 단건 소환: 상자 바로 앞쪽으로 툭 던짐
				TargetLoc += GetActorForwardVector() * 200.0f;
			}
			else
			{
				// 10연차 소환: 360도를 10등분하여 둥글게(방사형) 흩뿌림
				float AngleDegrees = (360.0f / ItemCount) * i;
				float Radius = 350.0f; // 퍼지는 원의 반지름 넓이

				// 삼각함수(Cos, Sin)를 이용한 원형 좌표 계산
				TargetLoc.X += FMath::Cos(FMath::DegreesToRadians(AngleDegrees)) * Radius;
				TargetLoc.Y += FMath::Sin(FMath::DegreesToRadians(AngleDegrees)) * Radius;
			}

			TargetLoc.Z = BoxLoc.Z; // 바닥 높이 유지

			// 3. 구슬에게 목표 지점으로 날아가라고 명령!
			// 약간의 시간차(FMath::RandRange)를 주면 더 자연스럽게 흩뿌려집니다.
			float RandomFlightTime = FMath::RandRange(0.8f, 1.2f);
			SpawnedItem->LaunchToTarget(TargetLoc, RandomFlightTime, 250.0f);
		}
	}
}
#pragma endregion 내부 유틸리티 및 이펙트 로직

