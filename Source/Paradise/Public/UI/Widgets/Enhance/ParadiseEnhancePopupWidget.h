// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseEnhancePopupWidget.generated.h"

#pragma region 전방 선언
class UParadiseSquadInventoryWidget;
class UParadiseEnhanceDetailWidget;
class UButton;
class UInventorySystem;
class UParadiseGameInstance;
#pragma endregion 전방 선언

/**
 * @class UParadiseEnhancePopupWidget
 * @brief 강화/돌파 시스템의 최상위 중재자 위젯
 * @details 인벤토리와 디테일 패널을 관리하며, 시스템(Model)에 강화/돌파를 요청합니다.
 */
UCLASS()
class PARADISE_API UParadiseEnhancePopupWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region 탭 제어
private:
	UFUNCTION() void OnClickCharTab();
	UFUNCTION() void OnClickWpnTab();
	UFUNCTION() void OnClickArmTab();
	UFUNCTION() void OnClickUnitTab();
	void SwitchTab(int32 NewTab);
#pragma endregion 탭 제어

#pragma region 시스템 연동 및 이벤트
private:
	/** @brief 재활용된 인벤토리에서 아이템 클릭 시 발생 */
	UFUNCTION()
	void HandleInventoryItemClicked(FSquadItemUIData ItemData);

	/** @brief 디테일 패널에서 강화 버튼 클릭 시 발생 */
	UFUNCTION()
	void RequestEnhance();

	/** @brief 디테일 패널에서 돌파 버튼 클릭 시 발생 */
	UFUNCTION()
	void RequestBreakthrough();

	UFUNCTION() 
	void HandleClose();

	/** @brief 현재 탭에 맞는 데이터를 InventorySystem에서 가져와 우측 패널을 갱신합니다. */
	void RefreshInventory();

	/** @brief 강화/돌파 후 화면을 최신 상태로 갱신합니다. */
	void RefreshAfterEnhancement();
#pragma endregion 시스템 연동 및 이벤트

#pragma region UI 컴포넌트 바인딩
protected:
	/** @brief 좌측 상세 정보 패널 (새로 만든 위젯) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseEnhanceDetailWidget> Panel_Detail = nullptr;

	/** @brief 우측 인벤토리 패널 (기존 편성창 위젯 재활용) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseSquadInventoryWidget> Panel_Inventory = nullptr;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Character = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Weapon = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Armor = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Unit = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Close = nullptr;
#pragma endregion UI 컴포넌트 바인딩

#pragma region 내부 상태 및 캐싱
private:
	int32 CurrentTabIndex = SquadTabs::Weapon;
	FSquadItemUIData SelectedItem;

	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;
	TWeakObjectPtr<UInventorySystem> CachedInventorySys = nullptr;
#pragma endregion 내부 상태 및 캐싱
};
