// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/GachaSubsystem.h"
#include "Engine/DataTable.h"
#include "Math/UnrealMathUtility.h"

void UGachaSubsystem::InitializeBanner(const FGachaBannerData& InBannerData)
{
	// 1. 배너 타입과 확률 세팅 캐싱
	CurrentBannerType = InBannerData.BannerType;
	CurrentRarityRates = InBannerData.RarityProbabilities;
	CurrentPityThreshold = InBannerData.PityThreshold;
	CachedRequiredAether = InBannerData.RequiredAether;

	//  에테르 비용 캐싱!
	CachedGachaPool.Empty();

	if (UDataTable* PoolTable = InBannerData.TargetPoolTable.LoadSynchronous())
	{
		TArray<FGachaPoolRow*> AllRows;
		PoolTable->GetAllRows<FGachaPoolRow>(TEXT("GachaInit"), AllRows);

		for (const FGachaPoolRow* Row : AllRows)
		{
			if (Row)
			{
				CachedGachaPool.FindOrAdd(Row->Rarity).Add(*Row);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[GachaSubsystem] 배너 초기화 완료 | 타입: %d | 천장: %d"),
			(int32)CurrentBannerType, CurrentPityThreshold);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaSubsystem] Pool 테이블 로드 실패!"));
	}
}

TArray<FGachaResult> UGachaSubsystem::PerformGacha(int32 PullCount, const TArray<FName>& OwnedItems)
{
	TArray<FGachaResult> Results;

	if (PullCount <= 0 || CachedGachaPool.IsEmpty() || CurrentRarityRates.IsEmpty())
	{
		return Results;
	}

	int32& PityStack = GetCurrentPityStackRef();

	for (int32 i = 0; i < PullCount; ++i)
	{
		// 1. 천장 카운터 증가
		PityStack++;
		const bool bIsPityTriggered = (PityStack >= CurrentPityThreshold);

		// 2. 등급 판정
		EItemRarity HitRarity = bIsPityTriggered
			? EItemRarity::Legendary
			: RollRarity(CurrentRarityRates);

		if (HitRarity == EItemRarity::Legendary)
		{
			PityStack = 0;
		}

		// 3. 아이템 추첨
		FGachaResult Result = PickItemFromRarity(HitRarity);

		// 4. 배너 타입별 중복 처리
		if (CurrentBannerType == EGachaBannerType::Equipment)
		{
			Result.bIsDuplicate = false;
			Result.ConvertedFragments = 0;
		}
		else
		{
			if (OwnedItems.Contains(Result.PulledItemID))
			{
				Result.bIsDuplicate = true;
				Result.ConvertedFragments = FMath::Max(10, Result.ConvertedFragments);
			}
			else
			{
				Result.bIsDuplicate = false;
				Result.ConvertedFragments = 0;
			}
		}

		Result.CurrentPityCount = PityStack;
		Results.Add(Result);
	}

	return Results;
}

int32 UGachaSubsystem::GetRemainingUntilPity() const
{
	return FMath::Max(0, CurrentPityThreshold - GetCurrentPityStackRef());
}

int32 UGachaSubsystem::GetCurrentPityStack() const
{
	return GetCurrentPityStackRef();
}

EItemRarity UGachaSubsystem::RollRarity(const TMap<EItemRarity, float>& Rates) const
{
	float TotalRate = 0.0f;
	for (const auto& Pair : Rates) TotalRate += Pair.Value;

	float RandomRoll = FMath::FRandRange(0.0f, TotalRate);

	for (const auto& Pair : Rates)
	{
		RandomRoll -= Pair.Value;
		if (RandomRoll <= 0.0f)
		{
			return Pair.Key;
		}
	}
	return Rates.begin().Key();
}

FGachaResult UGachaSubsystem::PickItemFromRarity(EItemRarity TargetRarity) const
{
	FGachaResult Result;
	Result.PulledRarity = TargetRarity;

	// 캐싱된 맵에서 해당 등급 서랍만 바로 엽니다 (최적화)
	const TArray<FGachaPoolRow>* RarityPool = CachedGachaPool.Find(TargetRarity);

	if (!RarityPool || RarityPool->IsEmpty())
	{
		Result.PulledItemID = NAME_None;
		return Result;
	}

	// 가중치 합산 및 픽업 계산
	float TotalWeight = 0.0f;
	for (const FGachaPoolRow& Item : *RarityPool) TotalWeight += Item.Weight;

	float RandomRoll = FMath::FRandRange(0.0f, TotalWeight);
	for (const FGachaPoolRow& Item : *RarityPool)
	{
		RandomRoll -= Item.Weight;
		if (RandomRoll <= 0.0f)
		{
			Result.PulledItemID = Item.ItemID;
			Result.ItemName = Item.ItemDisplayName;
			Result.ItemIcon = Item.ItemIcon.LoadSynchronous(); 
			Result.ItemCardIllust = Item.ItemCardIllust.LoadSynchronous();
			Result.CharacterSkeletalMesh = Item.CharacterSkeletalMesh.LoadSynchronous();
			Result.CharacterIdleAnim = Item.CharacterIdleAnim.LoadSynchronous();
			Result.RevealMeshScale = Item.RevealMeshScale;
			Result.ConvertedFragments = Item.DuplicateFragmentReward;
			break;
		}
	}

	return Result;
}

int32& UGachaSubsystem::GetCurrentPityStackRef()
{
	return (CurrentBannerType == EGachaBannerType::Equipment)
		? EquipmentPityStack
		: CharacterPityStack;
}

const int32& UGachaSubsystem::GetCurrentPityStackRef() const
{
	return (CurrentBannerType == EGachaBannerType::Equipment)
		? EquipmentPityStack
		: CharacterPityStack;
}