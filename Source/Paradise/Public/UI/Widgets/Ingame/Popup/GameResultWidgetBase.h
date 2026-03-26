// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameResultWidgetBase.generated.h"

#pragma region 전방 선언
class UButton;
#pragma endregion 전방 선언

/**
 * @class UGameResultWidgetBase
 * @brief 게임 결과 팝업(승리/패배)의 공통 기능을 담당하는 최상위 부모 클래스.
 * @details
 * 1. 로비로 이동하는 기능 (LevelLoadingSubsystem 활용).
 * 2. 현재 레벨을 재시작하는 기능.
 * 3. 추상 클래스(Abstract)로 설계하여 단독 생성을 방지함.
 */
UCLASS(Abstract) // 추상 클래스로 선언 (직접 사용 금지)
class PARADISE_API UGameResultWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

#pragma region 공통 UI 바인딩
protected:
	/** @brief 로비로 돌아가기 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Lobby = nullptr;

	/** @brief 현재 스테이지 재시작 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Retry = nullptr;
#pragma endregion 공통 UI 바인딩

#pragma region 내부 로직
protected:
	UFUNCTION()
	virtual void OnLobbyClicked();

	UFUNCTION()
	virtual void OnRetryClicked();

	/** @brief 딜레이 후 로비 전환 실행 */
	virtual void ExecuteLobby();

	/** @brief 딜레이 후 레벨 재시작 실행 */
	virtual void ExecuteRetry();
#pragma endregion 내부 로직

#pragma region 내부 데이터
private:
	/** @brief 로비 전환 딜레이 타이머 핸들 */
	FTimerHandle TimerHandle_Lobby;

	/** @brief 레벨 재시작 딜레이 타이머 핸들 */
	FTimerHandle TimerHandle_Retry;
#pragma endregion 내부 데이터
};
