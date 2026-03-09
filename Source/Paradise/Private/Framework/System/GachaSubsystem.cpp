// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/GachaSubsystem.h"
#include "Framework/System/ParadiseSaveGame.h"
#include "Engine/DataTable.h"
#include "Math/UnrealMathUtility.h"

void UGachaSubsystem::InitializeBanner(const FGachaBannerData& InBannerData)
{
	// 1. 배너 타입과 확률 세팅 캐싱
	CurrentBannerType = InBannerData.BannerType;
	CurrentRarityRates = InBannerData.RarityProbabilities;
	DefaultPityThreshold = InBannerData.PityThreshold;

	//  에테르 비용 캐싱!
	CachedRequiredAether = InBannerData.RequiredAether; 

	// 2. 연결된 풀(Pool) 테이블을 비동기/동기로 로드하여 캐싱
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
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaSubsystem] 타겟 풀(Pool) 테이블을 로드하지 못했습니다."));
	}
}

TArray<FGachaResult> UGachaSubsystem::PerformGacha(int32 PullCount, const TArray<FName>& OwnedItems)
{
	TArray<FGachaResult> Results;

	if (PullCount <= 0 || CachedGachaPool.IsEmpty() || CurrentRarityRates.IsEmpty())
	{
		return Results;
	}

	for (int32 i = 0; i < PullCount; ++i)
	{
		// 1. 천장(Pity) 카운터 증가
		CurrentPityStack++;
		bool bIsPityTriggered = (CurrentPityStack >= DefaultPityThreshold);

		// 2. 등급 판정
		EItemRarity HitRarity = bIsPityTriggered ? EItemRarity::Legendary : RollRarity(CurrentRarityRates);

		if (HitRarity == EItemRarity::Legendary)
		{
			CurrentPityStack = 0; // 전설 획득 시 천장 초기화
		}

		// 3. 아이템 뽑기 (여기서 Result.ConvertedFragments 에 기본값이 들어옴)
		FGachaResult Result = PickItemFromRarity(HitRarity);

		// ---------------------------------------------------------
		// 4. ★ 배너 타입에 따른 중복(Duplicate) 분기 처리 ★
		// ---------------------------------------------------------
		if (CurrentBannerType == EGachaBannerType::Equipment)
		{
			// 장비 배너: 장비는 중복의 개념이 없습니다. 무조건 온전한 장비로 지급됩니다.
			Result.bIsDuplicate = false;
			Result.ConvertedFragments = 0; // 조각 보상 없음!
		}
		else
		{
			// 캐릭터 배너: 보유 중인지 검사하여 조각으로 변환
			if (OwnedItems.Contains(Result.PulledItemID))
			{
				Result.bIsDuplicate = true;
				// 엑셀 기입 실수 방지용 최소치 보장
				Result.ConvertedFragments = FMath::Max(10, Result.ConvertedFragments);
			}
			else
			{
				Result.bIsDuplicate = false;
				Result.ConvertedFragments = 0;
			}
		}

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

void UGachaSubsystem::SaveToSaveGame(UParadiseSaveGame* SaveGameObj) const
{
	if (!SaveGameObj) return;

	SaveGameObj->SavedCurrentPityStack = CurrentPityStack;
}

void UGachaSubsystem::LoadFromSaveGame(UParadiseSaveGame* SaveGameObj)
{
	if (!SaveGameObj) return;

	CurrentPityStack = SaveGameObj->SavedCurrentPityStack;
}
