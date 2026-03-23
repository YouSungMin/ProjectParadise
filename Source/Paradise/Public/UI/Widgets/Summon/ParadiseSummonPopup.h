// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseSummonPopup.generated.h"

#pragma region 전방선언
class UButton;
class UWidgetSwitcher;
class UParadiseSummonPanel;
class UTextBlock;
class UParadiseGameInstance;
class UEconomySubsystem;
class ALobbyPlayerController;
class UParadiseResourceWarningWidget;
#pragma endregion 전방선언

/**
 * @brief 소환 시스템의 메인 팝업 위젯
 * @details WidgetSwitcher를 사용하여 탭(캐릭터/장비)에 따라 우측 컨텐츠를 교체합니다.
 * UI 최적화를 위해 위젯 스위처를 사용하여 불필요한 위젯 생성/파괴를 방지합니다.
 */
UCLASS()
class PARADISE_API UParadiseSummonPopup : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
	/** * @brief 서브시스템의 재화 변동 이벤트를 받아 UI를 갱신합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Summon")
	void RefreshCurrencyUI();
#pragma endregion 외부 인터페이스

#pragma region UI 컴포넌트
protected:
	// --- 좌측 패널 (탭 버튼) ---

	/** @brief 캐릭터 소환 탭 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Tab_Character = nullptr;

	/** @brief 장비 소환 탭 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Tab_Equipment = nullptr;

	// --- 상단 패널 (네비게이션 & 재화) ---

	/** @brief 로비로 돌아가는 뒤로가기 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Back = nullptr;

	/** @brief 현재 보유 에테르 표시 텍스트 (우측 상단 배치 권장) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_AetherAmount;

	// --- 우측 패널 (컨텐츠 영역) ---

	/** @brief 탭에 따라 내용을 교체해 보여줄 스위처 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_Content = nullptr;

	/** @brief 캐릭터 소환 패널 (WBP_SummonPanel_Character) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseSummonPanel> Panel_Character = nullptr;

	/** @brief 장비 소환 패널 (WBP_SummonPanel_Equipment) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseSummonPanel> Panel_Equipment = nullptr;

	/** @brief 에테르 부족 시 화면 전체를 덮을 경고 위젯 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseResourceWarningWidget> Widget_ResourceWarning = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 데이터 드리븐 설정
protected:
	/** @brief 경고 팝업에 넘겨줄 에테르 아이콘 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Icons")
	TSoftObjectPtr<UTexture2D> Icon_Aether = nullptr;
#pragma endregion 데이터 드리븐 설정

#pragma region 내부 상태 및 캐싱
private:
	/** @brief 가독성을 위한 인덱스 상수 (하드코딩 방지) */
	static constexpr int32 INDEX_CHARACTER = 0;
	static constexpr int32 INDEX_EQUIPMENT = 1;

	/** @brief 데이터 접근용 서브시스템 약참조 (순환 참조 및 메모리 릭 방지) */
	TWeakObjectPtr<UEconomySubsystem> CachedEconomySubsystem = nullptr;

	/** @brief 화면 전환(뒤로가기) 제어용 컨트롤러 약참조 */
	TWeakObjectPtr<ALobbyPlayerController> CachedPlayerController = nullptr;

	/** @brief 게임 저장(SaveData) 호출용 게임 인스턴스 약참조 */
	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;
#pragma endregion 내부 상태 및 캐싱

#pragma region 내부 로직
private:
	//EconomySubsystem의 재화 변경 방송을 수신할 전용 핸들러
	UFUNCTION()
	void HandleCurrencyChanged(ECurrencyType CurrencyType, int32 OldAmount, int32 NewAmount);

	/** @brief 캐릭터 탭 클릭 핸들러 */
	UFUNCTION()
	void OnCharacterTabClicked();

	/** @brief 장비 탭 클릭 핸들러 */
	UFUNCTION()
	void OnEquipmentTabClicked();

	/** @brief 뒤로가기 버튼 클릭 핸들러 */
	UFUNCTION()
	void OnBackButtonClicked();

	/** * @brief 탭 전환 시 UI 상태 업데이트 (버튼 활성/비활성 처리 등)
	 * @param NewIndex 전환할 스위처 인덱스
	 */
	void SwitchTab(int32 NewIndex);

	/** @brief 자식 패널에서 에테르 부족 이벤트가 오면 경고창을 띄우는 핸들러 */
	UFUNCTION()
	void HandleNotEnoughAether();
#pragma endregion 내부 로직

};
