// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SettingsSaveGame.generated.h"

/**
 * @class USettingsSaveGame
 * @brief 환경설정(볼륨 등)만 저장하는 전용 세이브 게임입니다.
 * @details ParadiseSaveGame과 완전히 독립적으로 작동하며,
 *          게임 진행 데이터와 별도로 "AudioSettings" 슬롯에 저장됩니다.
 */
UCLASS()
class PARADISE_API USettingsSaveGame : public USaveGame
{
	GENERATED_BODY()
	
#pragma region 데이터
public:
	/**
	 * @brief BGM 볼륨 (0.0 ~ 1.0).
	 */
	UPROPERTY()
	float BGMVolume = 0.7f;

	/**
	 * @brief SFX 볼륨 (0.0 ~ 1.0).
	 */
	UPROPERTY()
	float SFXVolume = 0.8f;
#pragma endregion 데이터
};
