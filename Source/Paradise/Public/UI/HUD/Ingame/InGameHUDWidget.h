// Copyright (C) Project Paradise. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InGameHUDWidget.generated.h"

#pragma region 전방 선언
class UActionControlPanel;
class USummonControlPanel;
class UPartyStatusPanel;
class UCharacterStatusWidget;
class UGameTimerWidget;
class UVirtualJoystickWidget;
class UResultPopupWidget;
class UVictoryPopupWidget;
class UDefeatPopupWidget;
class UParadiseCommonButton;
class AInGameGameState;
class USettingsPopupWidget;
#pragma endregion 전방 선언

/**
 * @class UInGameHUDWidget
 * @brief 인게임 화면의 최상위 HUD 컨테이너입니다.
 * @details Common UI의 ActivatableWidget을 상속받아 메뉴 팝업 시 입력 제어가 자동으로 처리됩니다.
 * 1. 하위 패널(Action, Summon, Party, Status)들을 컴포넌트로 관리합니다.
 * 2. GameState의 상태 변화(Ready -> Combat -> Victory/Defeat)를 감지하여 결과창을 띄우거나 타이머를 갱신합니다.
 * 3. 입력(조이스틱)을 캐릭터에게 전달하는 중계 역할을 수행할 수도 있습니다
 */
UCLASS()
class PARADISE_API UInGameHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

#pragma region 초기화
public:
	/** 
	 * @brief 게임 시작 시 각종 패널을 초기화하고 데이터를 연결합니다. 
	 * @details GameState와 연결하고 하위 위젯들을 초기 상태로 설정합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void InitializeHUD();

private:
	/** @brief GameState를 안전하게 캐싱하여 매 프레임 Cast 비용을 절약합니다. */
	TWeakObjectPtr<AInGameGameState> CachedGameState = nullptr;
#pragma endregion 초기화

#pragma region 하위 패널 접근 (Getters)
public:
	/** @brief 액션 패널(스킬, 공격) 반환 */
	FORCEINLINE UActionControlPanel* GetActionControlPanel() const { return ActionControlPanel; }

	/** @brief 소환 패널 반환 */
	FORCEINLINE USummonControlPanel* GetSummonControlPanel() const { return SummonControlPanel; }

	/** @brief 파티 상태 패널 반환 */
	FORCEINLINE UPartyStatusPanel* GetPartyStatusPanel() const { return PartyStatusPanel; }
#pragma endregion 하위 패널 접근 (Getters)

#pragma region 내부 로직
private:
	/** 
	 * @brief GameState의 Phase가 변경되었을 때 호출됩니다. (Delegate 바인딩)
	 * @param NewPhase 변경된 게임 단계
	 */
	UFUNCTION()
	void HandleGamePhaseChanged(EGamePhase NewPhase);

	/** @brief 설정 버튼 클릭 처리 */
	UFUNCTION()
	void OnSettingButtonClicked();

	/** @brief 자동/수동 전투 모드 전환 버튼 클릭 처리 */
	UFUNCTION()
	void OnAutoModeButtonClicked();

	/** @brief 가상 조이스틱 입력 처리 (캐릭터 이동) */
	UFUNCTION()
	void OnJoystickInput(FVector2D InputVector);

	/** @brief 주기적으로 호출될 UI 갱신 함수 */
	void OnUpdateHUD();
#pragma endregion 내부 로직

#pragma region 위젯 바인딩
private:
	/** @brief 좌측 상단 파티 목록 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPartyStatusPanel> PartyStatusPanel = nullptr;

	/** @brief 좌측 하단 가상 조이스틱 (이동) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVirtualJoystickWidget> VirtualJoystick = nullptr;

	/** @brief 우측 하단 스킬/공격 조작 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActionControlPanel> ActionControlPanel = nullptr;

	/** @brief 중앙 상단 게임 타이머 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGameTimerWidget> GameTimerWidget = nullptr;

	/** @brief 하단 중앙 소환수 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USummonControlPanel> SummonControlPanel = nullptr;

	/** @brief 우측 상단 자동 전투 토글 버튼 (설정 버튼 왼쪽에 배치) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> Btn_AutoMode = nullptr;

	/** @brief 우측 상단 설정 버튼 (CommonBtn) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> Btn_Setting = nullptr;

	/** @brief 승리 시 표시될 팝업 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVictoryPopupWidget> Widget_VictoryPopup = nullptr;

	/** @brief 패배 시 표시될 팝업 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDefeatPopupWidget> Widget_DefeatPopup = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 내부 데이터
private:
	/** @brief UI 갱신용 타이머 핸들 */
	FTimerHandle HUDUpdateTimerHandle;

	/** @brief 현재 자동 전투 활성화 여부 */
	bool bIsAutoMode = false;
#pragma endregion 내부 데이터

#pragma region 데이터 드리븐 설정
protected:
	/** @brief 기획자가 에디터에서 할당할 인게임용 설정 팝업 위젯 클래스 (WBP_Settings_InGame) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI")
	TSubclassOf<USettingsPopupWidget> SettingsPopupClass;
#pragma endregion 데이터 드리븐 설정

#pragma region 런타임 상태
private:
	/** @brief 화면에 미리 생성해둘 설정 팝업 위젯 인스턴스 */
	UPROPERTY()
	TObjectPtr<USettingsPopupWidget> SettingsPopupInstance = nullptr;
#pragma endregion 런타임 상태
};
