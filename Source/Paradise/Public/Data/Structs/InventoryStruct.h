// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Enums/GameEnums.h"
#include "InventoryStruct.generated.h"

/**
 * @brief 보유 영웅 데이터 (Level, Exp, ,장비장착 정보, 돌파 수치 등 성장 정보 포함)
 * @details 임시 구조체
 */
USTRUCT(BlueprintType)
struct FOwnedCharacterData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid CharacterUID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CharacterID; // 데이터 에셋 ID (RowName)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AwakeningLevel = 0; // 초월/각성 단계

	// Key: 장비 슬롯 (Weapon, Helmet 등)
	// Value: 장착된 아이템의 고유 ID (ItemUID)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EEquipmentSlot, FGuid> EquipmentMap;

};

/**
 * @brief 보유 퍼밀리어(병사) 데이터
 * @details 병사는 영웅처럼 개별 성장이 있을 수도, 단순히 수량(Quantity)만 관리할 수도 있습니다.
 * 일단은 영웅과 비슷하게 ID와 레벨을 갖는 구조로 잡았습니다. 임시 구조체
 */
USTRUCT(BlueprintType)
struct FOwnedFamiliarData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid FamiliarUID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FamiliarID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1; // 보유 수량 (병사는 여러 마리일 수 있음)
};

/**
 * @brief 보유 장비 데이터 (강화 수치 등 포함)
 * @details 임시 구조체
 */
USTRUCT(BlueprintType)
struct FOwnedItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid ItemUID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EnhancementLevel = 0; // 강화 수치 (+1, +2...)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1; // 갯수

	FOwnedItemData()
	{
		ItemUID = FGuid();
	}
};

