// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Data/Structs/InventoryStruct.h" 
#include "Data/Enums/GameEnums.h"      
#include "ParadiseSaveGame.generated.h"

/**
 * @brief 게임의 영구 저장 데이터를 담는 클래스
 */
UCLASS()
class PARADISE_API UParadiseSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UParadiseSaveGame();

	
protected:

private:


public:

#pragma region 저장 데이터 Default
	//세이브 슬롯 이름 (기본: "SaveSlot_01")
	UPROPERTY(VisibleAnywhere, Category = "Basic")
	FString SaveSlotName;

	// 기본 슬롯 이름
	const FString DefaultSaveSlot = TEXT("SaveSlot_01");

	//유저 인덱스 (기본: 0)
	UPROPERTY(VisibleAnywhere, Category = "Basic")
	uint32 UserIndex;
#pragma endregion 저장 데이터 Default

#pragma region 보유한 캐릭터 , 퍼밀리어 , 장비 데이터

	//인벤토리 데이터 (보유 아이템 목록)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TArray<FOwnedItemData> SavedOwnedInventoryItems;

	//보유 영웅 목록 (성장 정보 포함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TArray<FOwnedCharacterData> SavedOwnedCharacters;

	//보유 패밀리어 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TArray<FOwnedFamiliarData> SavedOwnedFamiliars;

#pragma endregion 보유한 캐릭터 , 퍼밀리어 , 장비 데이터


#pragma region 스쿼드 편성 데이터 

	// 🌟 스쿼드(편성) 저장 데이터
	// 영웅 편성 데이터 (3칸)
	UPROPERTY(VisibleAnywhere, Category = "SaveData|Squad")
	TArray<FName> SavedPlayerSquadIDs;

	// 퍼밀리어 편성 데이터 (5칸)
	UPROPERTY(VisibleAnywhere, Category = "SaveData|Squad")
	TArray<FName> SavedFamiliarSquadIDs;

#pragma endregion 스쿼드 편성 데이터

#pragma region 플레이어 재화

	UPROPERTY()
	TMap<ECurrencyType, int32> SavedWallet;


#pragma endregion 플레이어 재화

#pragma region 스테이지 해금 , 클리어 목록

	/** @brief 유저가 현재까지 입장 가능한(해금된) 스테이지 ID 목록 */
	UPROPERTY(VisibleAnywhere, Category = "SaveData|Stage")
	TArray<FName> SavedUnlockedStages;

	/** @brief 스테이지별 클리어 랭크(별 갯수). Key: 스테이지 ID, Value: 획득한 별(1~3) */
	UPROPERTY(VisibleAnywhere, Category = "SaveData|Stage")
	TMap<FName, int32> SavedStageClearStars;

	UPROPERTY(VisibleAnywhere, Category = "SaveData|Stage")
	int32 SavedMaxClearedStageIndex = 0;

#pragma endregion 스테이지 해금 , 클리어 목록
};
