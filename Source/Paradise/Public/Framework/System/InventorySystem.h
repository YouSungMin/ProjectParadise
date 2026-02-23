// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Structs/ItemStructs.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/InventoryStruct.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InventorySystem.generated.h"

#pragma region 델리게이트 선언

/**
 * @brief 인벤토리 변경 알림 델리게이트
 * @details 아이템이나 영웅 획득/소모 시 UI 갱신을 위해 호출됩니다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

/**
 * @brief 장비 상태 변경 알림 델리게이트
 * @details 장착/해제로 인해 장비 상태가 변했을 때 UI 갱신 등을 위해 호출됩니다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentUpdated);

#pragma endregion 델리게이트 선언

/**
 * @class UInventorySystem
 * @brief 플레이어(지휘관)의 자산(영웅, 병사, 장비)을 관리하는 컴포넌트
 * @details
 * - PlayerState에 부착되어 사용됩니다.
 * - GameInstance로부터 데이터를 받아 초기화(Init)합니다.
 * - 획득(Add) 및 소모(Remove) 시 데이터 테이블을 통해 ID 유효성을 검증합니다.
 */
UCLASS()
class PARADISE_API UInventorySystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:	
	UInventorySystem();

	#pragma region 장비 관련 함수 선언
	/**
	 * @brief 특정 캐릭터(UID 기반)에게 장비를 장착시킵니다.
	 * @param CharacterUID : 장비를 착용할 캐릭터의 고유 ID (FGuid)
	 * @param ItemUID : 장착할 아이템의 GUID
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	void EquipItemToCharacter(FGuid CharacterUID, FGuid ItemUID);

	/**
	 * @brief 특정 캐릭터(UID 기반)의 특정 슬롯 장비를 해제합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	void UnEquipItemFromCharacter(FGuid CharacterUID, EEquipmentSlot Slot);

	#pragma endregion 장비 관련 함수 선언

	#pragma region 인벤토리 관련 함수 선언
	/**
	 * @brief 계정 데이터로부터 인벤토리를 초기화하는 함수
	 * @details GameInstance(SaveFile)에 저장된 배열을 그대로 복사하여 가져옵니다.
	 * @param InHeroes 로드된 영웅 목록
	 * @param InFamiliars 로드된 퍼밀리어(병사) 목록
	 * @param InItems 로드된 장비/아이템 목록
	 */
	 UFUNCTION(BlueprintCallable, Category = "Inventory|Init")
	 void InitInventory(
		 const TArray<FOwnedCharacterData>& InHeroes,
		 const TArray<FOwnedFamiliarData>& InFamiliars,
		 const TArray<FOwnedItemData>& InItems
	 );

	/**
	 * @brief 아이템(퍼밀리어 , 장비)을 인벤토리에 추가하는 함수
	 * @note 내부적으로 ItemDataTable을 통해 유효한 ID인지 검증합니다.
	 * @param ItemID 추가할 아이템의 RowName (ID)
	 * @param Count 추가할 개수 (기본값 1)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Modify")
	void AddItem(FName ItemID, int32 Count = 1, int32 EnhancementLvl = 0);

	/**
	 * @brief 영웅을 획득하는 함수
	 * @details 이미 보유 중인 경우, 영혼석(조각)으로 대체하거나 레벨업 재료로 변환하는 로직이 추가될 수 있습니다.
	 * @param CharacterID 획득한 영웅의 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Modify")
	void AddCharacter(FName CharacterID);

	/**
	 * @brief 퍼밀리어을 획득하는 함수
	 * @param CharacterID 획득한 영웅의 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Modify")
	void AddFamiliar(FName FamiliarID);

	/** * @brief 통합 삭제 함수
		* @details 인벤토리 내의 영웅, 병사, 아이템을 모두 뒤져서 해당 GUID를 가진 객체를 삭제합니다.
		* @return 성공 시 true
		*/
	UFUNCTION(BlueprintCallable, Category = "Inventory|Unified")
	bool RemoveObjectByGUID(FGuid TargetGUID, int32 Count = 1);

	#pragma endregion 인벤토리 관련 함수 선언

	#pragma region 헬퍼 함수 선언

	/** @return 현재 보유 중인 모든 영웅 목록 (const 참조) */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	const TArray<FOwnedCharacterData>& GetOwnedCharacters() const { return OwnedCharacters; }

	/** @return 현재 보유 중인 모든 퍼밀리어 목록 (const 참조) */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	const TArray<FOwnedFamiliarData>& GetOwnedFamiliars() const { return OwnedFamiliars; }

	/** @return 현재 보유 중인 모든 아이템 목록 (const 참조) */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	const TArray<FOwnedItemData>& GetOwnedItems() const { return OwnedItems; }

	/**
	 * @brief GUID로 아이템 데이터 포인터 반환 (장비 장착 시 필수)
	 * @return 찾지 못하면 nullptr
	 */
	FOwnedItemData* GetItemByGUID(FGuid TargetUID);

	/**
	 * @brief 특정 아이템의 현재 보유 개수를 반환합니다.
	 * @param ItemID 확인할 아이템 ID
	 * @return int32 보유 수량 (없으면 0)
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	int32 GetItemQuantity(FName ItemID) const;

	/**
	 * @brief 특정 영웅을 보유하고 있는지 확인
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	bool HasCharacter(FName CharacterID) const;

	/**
	 * @brief ID로 보유 캐릭터의 상세 데이터를 (ID기반으로) 검색하여 반환합니다.
	 * @return 데이터를 찾으면 포인터 반환, 없으면 nullptr
	 */
	const FOwnedCharacterData* GetCharacterDataByID(FName CharacterID) const;

	/**
	 * @brief 아이템 ID를 기반으로 장착되어야 할 슬롯을 찾습니다.
	 * @details 무기/방어구 테이블을 조회하고 태그를 비교합니다.
	 */
	EEquipmentSlot FindEquipmentSlot(FName ItemID) const;

	#pragma endregion 헬퍼 함수 선언




private:
	/** @brief 내부 편의 함수: GameInstance 가져오기 */
	class UParadiseGameInstance* GetParadiseGI() const;


public:

#pragma region 델리게이트

	/** * @brief 인벤토리 변경 시 호출되는 델리게이트 (UI)
	 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryUpdated OnInventoryUpdated;


	/** * @brief 장비 변경 시 호출되는 이벤트 (UI 갱신용)
	 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEquipmentUpdated OnEquipmentUpdated;

#pragma endregion 델리게이트
protected:

#pragma region 인벤토리 보유 변수
	/** [영웅] 지휘관이 보유한 영웅들 (ID, Level, Awakening) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Storage")
	TArray<FOwnedCharacterData> OwnedCharacters;

	/** [퍼밀리어] 지휘관이 보유한 병사들 (ID, Level, Quantity) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Storage")
	TArray<FOwnedFamiliarData> OwnedFamiliars;

	/** [아이템] 보유한 장비 및 소모품 (ID, Enhancement, Quantity) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Storage")
	TArray<FOwnedItemData> OwnedItems;

#pragma endregion 인벤토리 보유 변수

private:

		
};
