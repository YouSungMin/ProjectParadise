// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseResourceWarningWidget.generated.h"

#pragma region 전방 선언
class UTextBlock;
class UImage;
class UButton;
class UWidgetAnimation;
class USoundBase;
#pragma endregion 전방 선언

/** @brief 경고 팝업이 닫혔음을 상위(Controller)에 알리는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarningPopupClosed);

/**
 * @class UParadiseResourceWarningWidget
 * @brief 재화(골드, 에테르, 조각 등) 부족 시 표시되는 범용 경고 팝업 (View)
 * @details 단일 책임 원칙(SRP)에 따라 경제 시스템(Economy)을 직접 참조하지 않고, 전달받은 텍스트와 아이콘만 렌더링합니다.
 */
UCLASS()
class PARADISE_API UParadiseResourceWarningWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 부족한 재화 정보를 받아 경고 팝업을 세팅하고 화면에 띄웁니다. (Data-Driven)
	 * @param ResourceName 부족한 재화의 이름 (예: "골드", "에테르", "아누비스 조각")
	 * @param ResourceIcon 부족한 재화의 아이콘 이미지
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Warning")
	void ShowWarning(const FText& ResourceName, UTexture2D* ResourceIcon);

	/** @brief 팝업 닫기 이벤트 브로드캐스트 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnWarningPopupClosed OnPopupClosed;
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/** @brief 확인/닫기 버튼 클릭 핸들러 */
	UFUNCTION()
	void HandleCloseButtonClicked();
#pragma endregion 내부 로직

#pragma region UI 컴포넌트 바인딩
protected:
	/** @brief 부족한 재화의 아이콘을 표시할 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_ResourceIcon = nullptr;

	/** @brief 메인 경고 메시지를 표시할 텍스트 (예: "골드가 부족합니다.") */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_WarningMessage = nullptr;

	/** @brief 팝업을 닫는 확인 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Confirm = nullptr;

	/** @brief 팝업 등장 시 재생할 통통 튀는 애니메이션 (UMG 애니메이션) */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_PopupAppear = nullptr;
#pragma endregion UI 컴포넌트 바인딩

#pragma region 데이터 드리븐 설정
protected:
	/** * @brief 경고 메시지 포맷 (기획자가 블루프린트에서 수정 가능)
	 * @details 기본값: "{0}이(가) 부족합니다!" ({0} 자리에 ResourceName이 치환됨)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Warning|Config")
	FText WarningMessageFormat = FText::FromString(TEXT("{0}이(가) 부족합니다!"));

	/** @brief 팝업이 뜰 때 재생할 에러/경고 사운드 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Warning|Config")
	TObjectPtr<USoundBase> Sound_Warning = nullptr;
#pragma endregion 데이터 드리븐 설정
};
