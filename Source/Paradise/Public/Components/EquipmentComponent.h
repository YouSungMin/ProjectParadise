// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Enums/GameEnums.h"
#include "Data/Structs/ItemStructs.h"
#include "Data/Structs/InventoryStruct.h"
#include "Components/ActorComponent.h"
#include "EquipmentComponent.generated.h"

class UDataTable;
class APlayerBase;
class UInventorySystem;



/**
 * @class UEquipmentComponent
 * @brief 캐릭터의 장착 상태 캐싱 및 3D 외형(Visual) 업데이트를 전담하는 뷰(View) 컴포넌트
 * @details
 * - **데이터 수동화 (Data-Driven):** 스스로 데이터를 조작하지 않으며, InventorySystem(Subsystem)의 데이터를 동기화합니다.
 * - **외형 갱신 (Visual Update):** 장착 정보를 바탕으로 실제 액터의 소켓에 무기를 부착하거나 방어구 메쉬를 교체합니다.
 * - **독립적 설계:** Subsystem에서 데이터를 직접 조회하므로 외부 주입 없이도 자가 초기화가 가능합니다.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARADISE_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEquipmentComponent();

	void TestEquip(APlayerBase* Char, EEquipmentSlot Slot, FName ItemID);

	/**
	 * @brief 인게임 스폰 시, 인벤토리 시스템의 데이터를 받아와 외형을 초기화합니다.
	 * @details 캐릭터 스폰 시 호출되며, 내부적으로 InventorySystem을 조회하여 장비 캐시를 갱신하고 메쉬를 생성합니다.
	 * @param InEquipmentMap 해당 캐릭터가 장착 중인 아이템의 GUID 맵
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Init")
	void InitializeEquipment(const TMap<EEquipmentSlot, FGuid>& InEquipmentMap);

	/**
	 * @brief 현재 특정 슬롯에 장착된 아이템 ID를 반환합니다.
	 * @return 아이템 ID (장착된 게 없으면 None)
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment|Query")
	FName GetEquippedItemID(EEquipmentSlot Slot) const;

	/**
	 * @brief 현재 슬롯에 장착된 아이템의 상세 데이터(강화수치, 수량 등)를 반환합니다.
	 * @return 데이터 찾음 여부
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment|Query")
	bool GetEquippedItemData(EEquipmentSlot Slot, FOwnedItemData& OutData) const;

	/**
	 * @brief (저장용) 현재 장착 중인 모든 장비의 GUID 맵을 반환합니다.
	 */
	const TMap<EEquipmentSlot, FGuid>& GetEquippedItems() const { return EquippedItems; }

	/**
	 * @brief 게임 전역 인벤토리 시스템(Subsystem) 인스턴스를 반환합니다.
	 * @return UInventorySystem 포인터 (월드나 게임인스턴스가 유효하지 않으면 nullptr)
	 */
	UInventorySystem* GetInventorySystem() const;

	/**
	 * @brief 현재 장비 상태에 맞춰 대상 캐릭터(육체)의 외형을 갱신합니다.
	 * @details
	 * - PlayerBase가 스폰되거나 빙의될 때(InitializePlayer) 호출됩니다.
	 * - 장비가 변경될 때도 내부적으로 호출됩니다.
	 * @param TargetCharacter 외형을 적용할 실제 캐릭터 액터
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Visual")
	void UpdateVisuals(APlayerBase* TargetCharacter);


	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestEquippedItem(EEquipmentSlot Slot, FName ItemID);
private:

	/**
	 * @brief 현재 장착 중인 장비들의 스탯을 긁어와서 플레이어의 Base 스탯에 더해줍니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Stats")
	void ApplyEquipmentStats();

	/**
	 * @brief (내부함수) 캐릭터의 스켈레탈 메시(방어구)를 교체합니다.
	 * @details 투구, 갑옷, 신발 등 부위별로 메시를 SetSkeletalMesh 합니다.
	 */
	void SetEquipmentMesh(APlayerBase* Char, EEquipmentSlot Slot, FName ItemID);
		
protected:

	/**
	 * @brief [핵심 변경] 현재 장착 중인 아이템 목록
	 * @details Key: 장착 슬롯 / Value: 아이템의 고유 식별자 (GUID)
	 * 기존 FName 대신 FGuid를 저장하여 특정 아이템(강화된 것)을 정확히 추적합니다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|State")
	TMap<EEquipmentSlot, FGuid> EquippedItems;

private:
	/** 현재 스폰된 무기 액터 (관리용) */
	UPROPERTY()
	TObjectPtr<AActor> SpawnedWeaponActor = nullptr;
};
