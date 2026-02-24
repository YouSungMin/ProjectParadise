// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Data/Enums/GameEnums.h"
#include "InGameGameState.generated.h"

/**
* @brief 게임 상태 변경 시 호출되는 다이나믹 멀티캐스트 델리게이트
* @param NewPhase 변경된 새로운 게임 단계
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EGamePhase, NewPhase);

/** 
* @brief 스테이지 타이머 변경 시 호출되는 다이나믹 멀티캐스트 델리게이트
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerChanged, int32, NewRemainingTime);

/**
 * @class AInGameGameState
 * @brief 인게임 플레이 중의 전역 상태(타이머, 페이즈, 보상 정보)를 관리하는 클래스입니다.
 */
UCLASS()
class PARADISE_API AInGameGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	//게임 진행 단계 설정 함수
	void SetCurrentPhase(EGamePhase NewPhase);

	/** @brief [진행] 스테이지 남은 시간 (UI 표시용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Data")
	float RemainingTime;

	/** @brief [진행] 스테이지 타이머의 활성화 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Data")
	bool bIsTimerActive = false;

	/** @brief [정보] 화면에 표시될 현재 스테이지 이름 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Data")
	FString DisplayStageName;

	/** @brief [보상] 이번 스테이지에서 획득한 누적 골드 (결과 UI용 & 캐릭터용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward")
	int32 AcquiredGold;

	/** @brief [보상] 이번 스테이지에서 획득한 누적 경험치 (결과 UI용 & 캐릭터용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward")
	int32 AcquiredExp;

	/** @brief [정보] 클리어 후 이동할 다음 스테이지 ID */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Data")
	FName NextStageID;

	/** @brief [이벤트] 게임 페이즈가 변경될 때 발생하는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnGamePhaseChanged OnGamePhaseChanged;

	/** @brief [이벤트] 타이머가 변경될 때 발생하는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTimerChanged OnTimerChanged;

	/** @brief [상태] 현재 진행 중인 게임 페이즈 (FSM 상태값) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Data")
	EGamePhase CurrentPhase;
};
