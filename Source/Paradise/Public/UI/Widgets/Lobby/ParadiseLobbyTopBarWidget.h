// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Enums/GameEnums.h"
#include "ParadiseLobbyTopBarWidget.generated.h"

#pragma region 전방 선언
class UTextBlock;
class UButton;
class USettingsPopupWidget;
class UEconomySubsystem;
#pragma endregion 전방 선언

/**
 * @class UParadiseLobbyTopBarWidget
 * @brief 로비 상단의 재화 정보 및 시스템 메뉴(설정, 나가기)를 표시하는 위젯.
 * @details EconomySubsystem의 델리게이트를 구독하여 재화 변경 시 즉각적으로 UI를 갱신합니다.
 */
UCLASS()
class PARADISE_API UParadiseLobbyTopBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma region UI 컴포넌트
protected:
	/** @brief 골드 표시 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GoldAmount = nullptr;

	/** @brief 에테르(가챠 재화) 표시 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_AetherAmount = nullptr;

	/** @brief 설정 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Settings = nullptr;

	/** @brief 게임 종료 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_QuitGame = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 재화 UI를 일괄 갱신합니다. (초기화 용도)
	 * @param InGold 갱신할 골드량
	 * @param InAether 갱신할 에테르량
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void UpdateCurrencyUI(int32 InGold, int32 InAether);
#pragma endregion 외부 인터페이스

#pragma region 내부 이벤트 핸들러 및 로직
private:
	/** * @brief 경제 서브시스템에서 재화 변동 방송이 올 때 호출됩니다. (Event-Driven)
	 */
	UFUNCTION()
	void HandleCurrencyChanged(ECurrencyType CurrencyType, int32 OldAmount, int32 NewAmount);

	/** @brief 골드 텍스트만 단독 갱신 (최적화) */
	void UpdateGoldText(int32 Amount);

	/** @brief 에테르 텍스트만 단독 갱신 (최적화) */
	void UpdateAetherText(int32 Amount);

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitGameClicked();
#pragma endregion 내부 이벤트 핸들러 및 로직

protected:
#pragma region 데이터 드리븐 설정
	/** @brief 기획자가 에디터에서 할당할 로비용 설정 팝업 위젯 클래스 (WBP_Settings_OutGame) */
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