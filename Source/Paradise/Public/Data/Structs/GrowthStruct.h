#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Data/Enums/GameEnums.h"
#include "GrowthStruct.generated.h"


/**
 * @struct FCharacterLevelUpData
 * @brief 캐릭터 레벨별 요구 경험치 및 누적 스탯 보너스를 정의하는 테이블 구조체입니다.
 * @note RowName은 반드시 해당 레벨의 숫자(예: "2", "3", "4")로 작성해야 코드에서 찾을 수 있습니다.
 */
USTRUCT(BlueprintType)
struct FCharacterLevelUpData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** @brief 다음 레벨로 넘어가기 위해 필요한 요구 경험치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp")
	int32 RequiredExp = 0;

	// --- [아래는 레벨업 시 캐릭터가 얻는 '누적' 추가 스탯입니다] ---

	/** @brief 현재 레벨에서의 보너스 최대 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp|Stats")
	float BonusMaxHP = 0.0f;

	/** @brief 현재 레벨에서의 보너스 공격력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp|Stats")
	float BonusAttackPower = 0.0f;

	/** @brief 현재 레벨에서의 보너스 방어력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp|Stats")
	float BonusDefense = 0.0f;
};

/**
 * @struct FEquipmentEnhanceData
 * @brief 장비 등급별 최대 강화 수치 및 성장 공식을 정의합니다.
 * @note RowName은 반드시 장비의 등급(예: "Normal", "Rare", "Epic")으로 작성해야 합니다.
 */
USTRUCT(BlueprintType)
struct FEquipmentEnhanceData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** @brief 해당 등급 장비의 최대 강화 도달 수치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance|Limit")
	int32 MaxEnhanceLevel = 10;

	/** @brief 기본 강화 비용 (0강 -> 1강 갈 때 필요한 골드) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance|Cost")
	int32 BaseGoldCost = 1000;

	/** @brief 레벨당 추가 요구 골드 (예: 500이면, 1->2강은 1500G, 2->3강은 2000G) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance|Cost")
	int32 GoldCostPerLevel = 500;

	/** * @brief 1강 당 증가하는 스탯 배율 (선형 증가)
	 * @details 예: 0.1f로 설정하면 1강당 기본 스탯의 10%씩 꾸준히 증가합니다. (5강 = +50%)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance|Stats")
	float StatBonusPerLevel = 0.1f;
};

/**
 * @struct FCharacterAwakenData
 * @brief 캐릭터 각성(돌파) 시 필요한 재화 및 해금되는 능력치를 정의합니다.
 */
USTRUCT(BlueprintType)
struct FCharacterAwakenData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** @brief 돌파에 필요한 캐릭터 영혼석(조각) 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awaken")
	int32 RequiredAwakeningPieces = 0;

	/** @brief 돌파에 필요한 골드 비용 */
	//현재는 골드가 들 기획은 아니지만 추후 확장시 구현가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awaken")
	int32 RequiredGold = 0;

	/** * @brief 돌파 시 해금되는 최대 레벨 상한
	 * @details 예: 0각성(만렙 30) -> 1각성 도달 시(만렙 40으로 확장)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awaken|Growth")
	int32 MaxLevelCap = 30;

	/**
	 * @brief 각성 시 뻥튀기되는 스탯 배율
	 * @details 레벨업과 별개로 캐릭터가 급격히 강해지는 배율입니다. (예: 1.2 = 전체 스탯 20% 증가)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awaken|Stats")
	float BonusStatMultiplier = 1.0f;
};