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
 * @brief 장비 강화 1회 시도에 필요한 비용 및 증가하는 스탯 배율을 정의합니다.
 */
USTRUCT(BlueprintType)
struct FEquipmentEnhanceData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** @brief 강화 비용 (골드) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	int32 RequiredGold = 0;

	/** @brief 요구 강화석 개수 */
	//일단 강화석 아이템 같은 건 없지만 0으로 해두고 추후 기능 확장시 구현
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	int32 RequiredMaterialCount = 0;

	/** * @brief 스탯 증가 배율 (누적)
	 * @details 무기의 기본 스탯에 곱해질 최종 배율입니다.
	 * 예: 1강(1.1) -> 기본 능력치의 110%, 2강(1.25) -> 기본 능력치의 125%
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance|Stats")
	float StatMultiplier = 1.0f;
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