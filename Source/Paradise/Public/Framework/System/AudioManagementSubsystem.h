// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AudioManagementSubsystem.generated.h"

#pragma region 전방 선언
class USoundBase;
class UAudioComponent;
#pragma endregion 전방 선언

/**
 * @class UAudioManagementSubsystem
 * @brief 게임 내 배경음(BGM)의 재생 및 흐름을 전담 관리하는 시스템입니다.
 * @details 위젯이 파괴되어도 음악이 끊기지 않게 하며, 페이드 연출을 전역적으로 제어합니다.
 */
UCLASS()
class PARADISE_API UAudioManagementSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/**
	 * @brief 새로운 배경음악을 재생합니다.
	 * @param NewBGM 재생할 사운드 에셋
	 * @param bLoop 반복 재생 여부
	 * @param FadeTime 페이드 인 시간
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	void PlayBGM(USoundBase* NewBGM, bool bLoop = true, float FadeTime = 1.0f);

	/**
	 * @brief 현재 재생 중인 배경음악을 부드럽게 정지합니다.
	 * @param FadeTime 페이드 아웃 시간
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	void StopBGM(float FadeTime = 1.0f);

private:
	/** @brief 현재 재생 중인 BGM 컴포넌트 (GameInstance 수명 주기 동안 유지) */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveBGMComponent = nullptr;
};
