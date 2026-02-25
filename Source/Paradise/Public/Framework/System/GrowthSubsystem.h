// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GrowthSubsystem.generated.h"

/**
 * @class UGrowthSubsystem
 * @brief 캐릭터 레벨업, 각성(돌파), 장비 강화 등 '성장 로직'을 전담하는 시스템
 */
UCLASS()
class PARADISE_API UGrowthSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/** @brief 캐릭터 경험치 획득 및 레벨업 계산 */
	UFUNCTION(BlueprintCallable, Category = "Growth|Level")
	void AddCharacterExp(FName CharacterID, int32 ExpAmount);

	/** @brief 중복 캐릭터 획득 시 조각(피스) 변환 로직 처리 */
	UFUNCTION(BlueprintCallable, Category = "Growth|Awaken")
	void HandleDuplicateCharacter(FName CharacterID);

	/** @brief 캐릭터 돌파(각성) 시도 */
	UFUNCTION(BlueprintCallable, Category = "Growth|Awaken")
	bool AwakenCharacter(FName CharacterID);

	/** @brief 장비 강화 시도 */
	UFUNCTION(BlueprintCallable, Category = "Growth|Equipment")
	bool EnhanceEquipment(FGuid ItemUID);
};
