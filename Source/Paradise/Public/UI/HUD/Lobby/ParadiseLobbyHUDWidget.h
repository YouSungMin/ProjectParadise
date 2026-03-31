// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Enums/ParadiseLobbyEnums.h"
#include "ParadiseLobbyHUDWidget.generated.h"

#pragma region 전방 선언
class UWidgetSwitcher;
class UParadiseLobbyTopBarWidget;
class UParadiseLobbyMenuPanelWidget;
class ALobbyPlayerController;
class UParadiseGameInstance;
class UAudioManagementSubsystem;
class UParadiseFXAudioData;
class USettingsPopupWidget;
#pragma endregion 전방 선언

/**
 * @class UParadiseLobbyHUDWidget
 * @brief 로비 UI의 최상위 컨테이너 (View).
 * @details
 * 1. 데이터(Data-Driven)에 기반하여 하위 패널 위젯을 생성합니다.
 * 2. Controller로부터 상태 변경 알림을 받아 Switcher를 조작합니다.
 */
UCLASS()
class PARADISE_API UParadiseLobbyHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
#pragma region UI 컴포넌트
protected:
	/** @brief 상단 재화 및 설정 바 (WBP_TopBar) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseLobbyTopBarWidget> WBP_TopBar = nullptr;

	/** @brief 우측 중앙 메뉴 버튼 패널 (WBP_MenuPanel) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseLobbyMenuPanelWidget> WBP_MenuPanel = nullptr;

	/** @brief 메뉴별 콘텐츠를 보여주는 스위처 (전투, 소환 창 등) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_Content = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 데이터 설정 (Data-Driven)
protected:
	/**
	 * @brief 메뉴별 생성할 위젯 클래스 목록.
	 * @details BP에서 Key(Enum) - Value(WidgetClass) 매핑. None은 비워둡니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Config")
	TMap<EParadiseLobbyMenu, TSubclassOf<UUserWidget>> MenuWidgetClasses;
#pragma endregion 데이터 설정

#pragma region 내부 캐싱
private:
	/** @brief 생성된 메뉴 위젯 캐싱 (Object Pooling 효과). */
	UPROPERTY()
	TMap<EParadiseLobbyMenu, TObjectPtr<UUserWidget>> CreatedMenuWidgets;

	/**
	 * @brief [최적화] 매번 컨트롤러를 찾는 연산 비용을 없애기 위한 약참조 캐싱
	 * @details 순환 참조 방지를 위해 TWeakObjectPtr를 사용합니다.
	 */
	TWeakObjectPtr<ALobbyPlayerController> CachedController = nullptr;

	/** @brief 매번 게임 인스턴스를 캐스팅하는 비용을 줄이기 위한 캐싱 */
	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;
#pragma endregion 내부 캐싱

#pragma region 외부 제어
public:
	/**
	 * @brief 컨트롤러에 의해 호출되어 화면을 갱신합니다.
	 * @param InCurrentMenu 변경된 메뉴 상태.
	 */
	void UpdateMenuStats(EParadiseLobbyMenu InCurrentMenu);

	/** @brief 카메라 이동 시작 시 호출 (모든 UI 페이드 아웃) */
	void OnStartCameraMove();

	/**
	 * @brief 컨트롤러의 ESC 입력 또는 화면 설정 버튼 클릭 시 설정 팝업을 토글합니다.
	 * @details 내부의 USettingsPopupWidget 인스턴스에 접근하여 열림/닫힘(Toggle)을 위임합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void ToggleSettingsPopup();

	/**
	 * @brief 설정 팝업 인스턴스를 반환합니다.
	 * @details LobbyController의 ESC 입력 처리 시 호출합니다.
	 */
	FORCEINLINE class USettingsPopupWidget* GetSettingsPopupInstance() const { return SettingsPopupInstance; }

private:
	/**
	 * @brief 여러 팝업의 뒤로가기 요청을 하나로 통합 처리합니다.
	 * (메뉴로 복귀하는 단일 책임)
	 */
	UFUNCTION()
	void HandleBackToMainLobby();
#pragma endregion 외부 제어

protected:
	/** @brief 기획자가 에디터에서 할당할 로비용 설정 팝업 위젯 클래스 (WBP_Settings_Lobby) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI")
	TSubclassOf<USettingsPopupWidget> SettingsPopupClass;

private:
	/** @brief 화면에 미리 생성해둘 설정 팝업 위젯 인스턴스 */
	UPROPERTY()
	TObjectPtr<USettingsPopupWidget> SettingsPopupInstance = nullptr;

};