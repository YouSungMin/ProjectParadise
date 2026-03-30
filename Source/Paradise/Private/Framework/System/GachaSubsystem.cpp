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
	CurrentPityThreshold = InBannerData.PityThreshold;

	//  에테르 비용 캐싱!
	CachedRequiredAether = InBannerData.RequiredAether; 

	CachedGachaPool.Empty();

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
	/*else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GachaSubsystem] 타겟 풀(Pool) 테이블을 로드하지 못했습니다."));
	}*/
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
		++PityStack;
		const bool bIsPityTriggered = (PityStack >= CurrentPityThreshold);

		// 2. 등급 판정 (천장 도달 시 최고 등급 확정)
		const EItemRarity HitRarity = bIsPityTriggered
			? EItemRarity::Legendary
			: RollRarity(CurrentRarityRates);

		if (HitRarity == EItemRarity::Legendary)
		{
			PityStack = 0;
		}

		// 3. 아이템 추첨
		FGachaResult Result = PickItemFromRarity(HitRarity);

		// 4. 중복 여부 플래그 — UI 연출 분기 전용 (bIsDuplicate)
		//    실제 보상 지급(조각/에테르)은 AddCharacter →
		//    GrowthSubsystem::HandleDuplicateCharacter 가 전담합니다.
		if (CurrentBannerType == EGachaBannerType::Character)
		{
			Result.bIsDuplicate = OwnedItems.Contains(Result.PulledItemID);
		}
		// 장비 배너는 중복 개념 없음 — bIsDuplicate 기본값 false 유지

		Result.CurrentPityCount = PityStack;
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

	// 현재 배너 타입과 일치하는 항목만 필터링
	TArray<const FGachaPoolRow*> FilteredPool;
	for (const FGachaPoolRow& Item : *RarityPool)
	{
		if (Item.ItemBannerType == CurrentBannerType)
		{
			FilteredPool.Add(&Item);
		}
	}

	if (FilteredPool.IsEmpty())
	{
		UE_LOG(LogTemp, Error,
			TEXT("❌ [GachaSubsystem] 배너 타입[%d]에 맞는 항목이 풀에 없습니다. 데이터 테이블을 확인하세요."),
			static_cast<int32>(CurrentBannerType));
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
			// 결과 구조체에 필요한 데이터 복사
			Result.PulledItemID = Item.ItemID;
			Result.ItemName = Item.ItemDisplayName;
			Result.ItemIcon = Item.ItemIcon.LoadSynchronous();
			Result.ItemCardIllust = Item.ItemCardIllust.LoadSynchronous();
			Result.RevealMeshScale = Item.RevealMeshScale;

			// 캐릭터 전용 필드
			Result.CharacterSkeletalMesh = Item.CharacterSkeletalMesh.LoadSynchronous();
			Result.CharacterIdleAnim = Item.CharacterIdleAnim.LoadSynchronous();

			// 장비 전용 필드
			Result.EquipmentStaticMesh = Item.EquipmentStaticMesh.LoadSynchronous();

			// SkeletalMesh 가 있으면 캐릭터, 없으면 장비로 판단
			Result.bIsCharacter = (Result.CharacterSkeletalMesh != nullptr);
			break;
		}
	}

	return Result;
}

int32 UGachaSubsystem::GetCurrentPityStack() const
{
	return GetCurrentPityStackRef();
}

int32 UGachaSubsystem::GetRemainingUntilPity() const
{
	return FMath::Max(0, CurrentPityThreshold - GetCurrentPityStackRef());
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

void UGachaSubsystem::SaveToSaveGame(UParadiseSaveGame* SaveGameObj) const
{
	if (!SaveGameObj) return;

	SaveGameObj->SavedCurrentPityStack = CharacterPityStack;
	// 장비 천장도 저장
}

void UGachaSubsystem::LoadFromSaveGame(UParadiseSaveGame* SaveGameObj)
{
	if (!SaveGameObj) return;

	CharacterPityStack = SaveGameObj->SavedCurrentPityStack;
	// 장비 천장도 저장
}
