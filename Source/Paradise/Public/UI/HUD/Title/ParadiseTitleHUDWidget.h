// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "ParadiseTitleHUDWidget.generated.h"

#pragma region 전방 선언
class UButton;
class UWidgetAnimation;
class USettingsPopupWidget;
class UParadiseFXAudioData;
#pragma endregion 전방 선언

/**
 * @class UParadiseTitleHUDWidget
 * @brief 게임 타이틀 화면(Touch to Start)을 관리하는 메인 HUD 위젯입니다.
 * @details
 * 1. 전체 화면 터치 버튼을 통해 게임 시작 입력을 받습니다.
 * 2. 'Press Any Key 또는 Touch to Start' 텍스트 깜빡임 애니메이션을 재생합니다.
 * 3. ParadiseGameInstance를 통해 로비(Lobby) 레벨로 비동기 로딩을 요청합니다.
 * 4. 종료 및 설정 버튼 기능을 제공합니다.
 */
UCLASS()
class PARADISE_API UParadiseTitleHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 설정 팝업 인스턴스를 반환합니다.
	 * @details TitleController의 ESC 입력 처리 시 호출합니다.
	 */
	FORCEINLINE USettingsPopupWidget* GetSettingsPopupInstance() const { return SettingsPopupInstance; }
#pragma endregion 외부 인터페이스

#pragma region 설정 데이터
protected:
	/** 
	 * @brief 로비로 이동할 때 미리 로딩할 에셋 목록 (Soft Reference).
	 * @details 기획자가 에디터에서 텍스처, 데이터 테이블 등을 등록하면 로딩 바 진행률에 반영됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paradise|Config")
	TArray<TSoftObjectPtr<UObject>> PreloadAssets;

	/** @brief 이동할 레벨의 이름 (기본값: L_Lobby/ 일단 테스트용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paradise|Config")
	FName NextLevelName = FName("L_Lobby");
#pragma endregion 설정 데이터

#pragma region 위젯 바인딩
private:
	/** @brief 화면 전체를 덮는 투명 버튼 (Touch Input) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_ScreenTouch = nullptr;

	/** @brief 게임 종료 버튼 (최우측 상단 등 배치 예정) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit = nullptr;

	/** @brief 설정 버튼 (종료 버튼 왼쪽에 배치 예정) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Settings = nullptr;

	/** @brief 'Touch to Start' 텍스트 깜빡임 애니메이션 */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_BlinkText = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 내부 로직
private:
	/** @brief 화면 터치 시 호출되는 핸들러 */
	UFUNCTION()
	void OnScreenTouched();

	/** @brief 종료 버튼 클릭 시 호출되는 핸들러 */
	UFUNCTION()
	void OnQuitButtonClicked();

	/** @brief 설정 버튼 클릭 시 호출되는 핸들러 */
	UFUNCTION()
	void OnSettingsButtonClicked();

	/**
	 * @brief 터치 연출(효과음, 페이드아웃)이 끝난 후 실제 로딩을 지시하는 헬퍼 함수
	 * @details OnScreenTouched에서 타이머를 통해 지연 호출됩니다.
	 */
	void ExecuteLevelTransition();

	/** @brief 중복 로딩 방지용 플래그 */
	bool bIsLoadingStarted = false;
#pragma endregion 내부 로직
protected:

#pragma region 데이터 드리븐 설정
	/** @brief 기획자가 에디터에서 할당할 타이틀용 설정 팝업 위젯 클래스 (WBP_Settings_OutGame) */
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
