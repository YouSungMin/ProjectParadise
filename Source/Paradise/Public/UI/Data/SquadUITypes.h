// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/Enums/GameEnums.h"
#include "SquadUITypes.generated.h"

class UTexture2D;

#pragma region 열거형 정의
/** @brief UI 상태 정의 (일반 모드 vs 장비 교체 모드) */
UENUM(BlueprintType)
enum class ESquadUIState : uint8
{
	Normal,			// 일반 상태 (상세 정보만 표시)
	CharacterSwap,	// 캐릭터/유닛 교체 모드
	EquipmentSwap	// 장비 교체 모드
};

/**
 * @enum ESquadDetailContext
 * @brief 상세 정보창이 어떤 슬롯을 클릭해서 열렸는지 정의하는 컨텍스트
 */
UENUM(BlueprintType)
enum class ESquadDetailContext : uint8
{
	FormationCharacter,  /**< 편성창 메인/서브 캐릭터 슬롯 */
	FormationUnit,       /**< 편성창 유닛 슬롯 */
	InventoryCharacter,  /**< 인벤토리 캐릭터 슬롯 */
	InventoryWeapon,     /**< 인벤토리 무기 슬롯 */
	InventoryArmor,      /**< 인벤토리 방어구 슬롯 */
	InventoryUnit        /**< 인벤토리 유닛 슬롯 */
};

/** @brief 탭 인덱스 상수 (가독성용) */
namespace SquadTabs
{
	const int32 Character = 0;
	const int32 Weapon = 1;
	const int32 Armor = 2;
	const int32 Unit = 3;
	const int32 Misc = 4;
}
#pragma endregion 열거형 정의

#pragma region UI 데이터 구조체
/** 
 * @brief 리스트/슬롯 표시에 필요한 순수 데이터 (View Model)
 * @details 인벤토리(ID, 레벨)와 데이터테이블(아이콘, 등급, 이름)을 합친 정보입니다.
 */
USTRUCT(BlueprintType)
struct FSquadItemUIData
{
	GENERATED_BODY()

	/**
	 * @brief [핵심] 인벤토리에 존재하는 고유 인스턴스 ID (FGuid)
	 * @details 장착, 강화, 판매 등 시스템에 실제 명령을 내릴 때 반드시 필요한 식별자입니다.
	 */
	UPROPERTY(BlueprintReadOnly)
	FGuid InstanceUID;

	/** @brief 아이템/캐릭터 원본 데이터 테이블 ID */
	UPROPERTY(BlueprintReadOnly)
	FName ID = NAME_None;

	/** @brief 표시 이름 */
	UPROPERTY(BlueprintReadOnly)
	FText Name;

	/** @brief 아이콘 이미지 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** @brief 아이템/유닛의 등급 (UI 테두리 색상 결정에 사용) */
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	EItemRarity Rarity = EItemRarity::Common;

	/** @brief 레벨 */
	UPROPERTY(BlueprintReadOnly)
	int32 Level = 0;

	/** @brief 보유 수량 (무기나 장비 등 겹칠 수 있는 아이템용) */
	UPROPERTY(BlueprintReadOnly)
	int32 Quantity = 1;

	/** @brief 현재 편성에 장착 중인지 여부 (인벤토리 테두리 표시용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEquipped = false;

	/** @brief 교체를 위해 선택된 상태인지 여부 (인벤토리 하이라이트용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsSelected = false;

	/** @brief 도감용: 유저가 이 항목을 획득(보유)했는지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsOwned = true; // 기본값은 true (인벤토리에서는 다 보유한 것만 넘기니까)
};
#pragma endregion UI 데이터 구조체