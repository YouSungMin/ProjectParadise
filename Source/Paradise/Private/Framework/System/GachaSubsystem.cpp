// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/GachaSubsystem.h"
#include "Engine/DataTable.h"
#include "Math/UnrealMathUtility.h"

void UGachaSubsystem::InitializeBanner(UDataTable* BannerTable)
{
	if (!BannerTable || CurrentBannerTable == BannerTable) return;

	CurrentBannerTable = BannerTable;
	CachedGachaPool.Empty();

	// 테이블 로드 후 등급별로 분류하여 캐싱 (O(1) 검색 최적화)
	TArray<FGachaPoolRow*> AllRows;
	BannerTable->GetAllRows<FGachaPoolRow>(TEXT("GachaInit"), AllRows);

	for (const FGachaPoolRow* Row : AllRows)
	{
		if (Row)
		{
			CachedGachaPool.FindOrAdd(Row->Rarity).Add(*Row);
		}
	}
}

TArray<FGachaResult> UGachaSubsystem::PerformGacha(int32 PullCount, const TMap<EItemRarity, float>& RarityRates, const TArray<FName>& OwnedItems)
{
	TArray<FGachaResult> Results;

	if (PullCount <= 0 || CachedGachaPool.IsEmpty() || RarityRates.IsEmpty())
	{
		return Results;
	}

	for (int32 i = 0; i < PullCount; ++i)
	{
		// 1. 천장(Pity) 카운터 증가
		CurrentPityStack++;
		bool bIsPityTriggered = (CurrentPityStack >= DefaultPityThreshold);

		// 2. 등급 판정 (천장에 도달했으면 강제 전설, 아니면 확률 추첨)
		EItemRarity HitRarity = bIsPityTriggered ? EItemRarity::Legendary : RollRarity(RarityRates);

		// ★ 전설이 뽑혔다면 (운이 좋든 천장이든) 스택 리셋
		if (HitRarity == EItemRarity::Legendary)
		{
			CurrentPityStack = 0;
		}

		// 3. 해당 등급 내에서 아이템 뽑기 (이 안에서 이미 Item.DuplicateFragmentReward 값이 세팅되어 나옴)
		FGachaResult Result = PickItemFromRarity(HitRarity);

		// 4. 중복 검사 및 파편 처리
		if (OwnedItems.Contains(Result.PulledItemID))
		{
			Result.bIsDuplicate = true;

			// 기획자가 데이터 테이블에 실수로 파편 보상량을 0이나 음수로 넣었을 때만 10개(최소 보장)로 세팅
			if (Result.ConvertedFragments <= 0)
			{
				Result.ConvertedFragments = 10;
			}
		}
		else
		{
			Result.bIsDuplicate = false;
			// 중복이 아니므로 조각은 주지 않음
			Result.ConvertedFragments = 0;
		}

		// UI에서 표기할 천장 스택 기록
		Result.CurrentPityCount = CurrentPityStack;

		Results.Add(Result);
	}

	return Results;
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
			// 여기서 중복 보상량을 미리 세팅해 둡니다.
			Result.ConvertedFragments = Item.DuplicateFragmentReward;
			break;
		}
	}

	return Result;
}
