// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Structs/GachaTypes.h"
#include "GachaSubsystem.generated.h"

#pragma region 전방 선언
class UDataTable;
#pragma endregion 전방 선언

/**
 * @class UGachaSubsystem
 * @brief 가챠 확률 계산 및 천장(Pity), 중복 조각 변환 로직을 전담하는 백엔드 코어
 * @details 캐싱을 통한 최적화로 100연차를 돌려도 프레임 드랍이 발생하지 않습니다.
 */
UCLASS()
class PARADISE_API UGachaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 새로운 배너의 마스터 데이터를 등록하고 확률 풀을 캐싱합니다.
	 * @param InBannerData 기획자가 세팅한 배너 데이터 구조체
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha")
	void InitializeBanner(const FGachaBannerData& InBannerData);

	/**
	 * @brief 지정된 횟수만큼 가챠를 돌리고 결과를 반환합니다.
	 * @param PullCount   뽑기 횟수
	 * @param OwnedItems  보유 중인 캐릭터 ID 목록
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha")
	TArray<FGachaResult> PerformGacha(int32 PullCount, const TArray<FName>& OwnedItems);

	/** @brief 현재 배너의 1회 뽑기 에테르 요구량 */
	UFUNCTION(BlueprintPure, Category = "Paradise|Gacha")
	int32 GetCurrentAetherRequirement() const { return CachedRequiredAether; }

	/**
	 * @brief 현재 배너 타입의 천장까지 남은 횟수를 반환합니다.
	 * @details UI에서 "천장까지 N회" 표시에 사용합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "Paradise|Gacha")
	int32 GetRemainingUntilPity() const;

	/**
	 * @brief 현재 배너 타입의 현재 천장 스택을 반환합니다.
	 * @details UI 디버그 표시용
	 */
	UFUNCTION(BlueprintPure, Category = "Paradise|Gacha")
	int32 GetCurrentPityStack() const;
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/** @brief 확률에 따라 등급을 추첨합니다. */
	EItemRarity RollRarity(const TMap<EItemRarity, float>& Rates) const;

	/** @brief 캐싱된 풀에서 가중치 기반으로 아이템을 추첨합니다. */
	FGachaResult PickItemFromRarity(EItemRarity TargetRarity) const;

	/** @brief 현재 배너 타입에 맞는 천장 스택 레퍼런스를 반환합니다. */
	int32& GetCurrentPityStackRef();
	const int32& GetCurrentPityStackRef() const;
#pragma endregion 내부 로직

#pragma region 내부 데이터
private:
	/** @brief 현재 활성화된 배너의 종류 */
	EGachaBannerType CurrentBannerType = EGachaBannerType::Character;

	/** @brief 현재 배너의 등급별 확률표 */
	TMap<EItemRarity, float> CurrentRarityRates;

	/** @brief 등급별로 캐싱된 데이터 풀 */
	TMap<EItemRarity, TArray<FGachaPoolRow>> CachedGachaPool;

	/** @brief 캐릭터 배너 전용 천장 스택 */
	UPROPERTY()
	int32 CharacterPityStack = 0;

	/** @brief 장비 배너 전용 천장 스택 */
	UPROPERTY()
	int32 EquipmentPityStack = 0;

	/** @brief 현재 배너의 천장 임계값 */
	int32 CurrentPityThreshold = 80;

	/** @brief 현재 배너 1회 뽑기 에테르 비용 */
	int32 CachedRequiredAether = 0;
#pragma endregion 내부 데이터
};