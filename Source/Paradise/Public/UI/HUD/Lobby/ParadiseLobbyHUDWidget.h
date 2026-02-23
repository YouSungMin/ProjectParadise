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
#pragma endregion 내부 캐싱

#pragma region 외부 제어 (From Controller)
public:
	/**
	 * @brief 컨트롤러에 의해 호출되어 화면을 갱신합니다.
	 * @param InCurrentMenu 변경된 메뉴 상태.
	 */
	void UpdateMenuStats(EParadiseLobbyMenu InCurrentMenu);

	/** @brief 카메라 이동 시작 시 호출 (모든 UI 페이드 아웃) */
	void OnStartCameraMove();

private:
	/** @brief 편성(Squad) 위젯에서 뒤로가기 버튼을 눌렀을 때 호출되는 콜백 */
	UFUNCTION()
	void HandleSquadBackRequest();
#pragma endregion 외부 제어
};