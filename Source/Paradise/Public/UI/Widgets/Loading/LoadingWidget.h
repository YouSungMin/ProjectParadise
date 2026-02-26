// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingWidget.generated.h"

#pragma region 전방 선언
class UProgressBar;
class UTextBlock;
class UImage;
#pragma endregion 전방 선언

/**
 * @class ULoadingWidget
 * @brief 비동기 로딩 진행률을 시각적으로 표시하는 위젯 클래스.
 * @details ParadiseGameInstance에서 계산된 로딩 퍼센트(0.0 ~ 1.0)를 받아 ProgressBar와 텍스트를 갱신합니다.
 */
UCLASS()
class PARADISE_API ULoadingWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	ULoadingWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 로딩 진행률을 설정하고 UI를 갱신합니다.
	 * @param Percent 진행률 (0.0 ~ 1.0 범위)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void SetLoadingPercent(float Percent);

	/**
	 * @brief 로딩 팁이나 상태 메시지를 변경합니다.
	 * @param NewText 표시할 텍스트
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void SetLoadingText(FText NewText);

	/**
	 * @brief 로딩 배경 이미지를 교체합니다.
	 * @param InTexture 교체할 배경 텍스처 (nullptr이면 기본 설정 유지)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void SetBackgroundImage(UTexture2D* InTexture);
#pragma endregion 외부 인터페이스

#pragma region 이벤트 (블루프린트 확장)
protected:
	/** 
	 * @brief 로딩이 완료되었을 때(100%) 호출되는 이벤트. 
	 * @details 블루프린트에서 페이드 아웃 애니메이션 등을 구현 (일부러 BlueprintImplementableEvent 써서 기획자가 알아서 하도록 만들었음)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Paradise|UI|Loading")
	void OnLoadingComplete();
#pragma endregion 이벤트 (블루프린트 확장)

#pragma region 위젯 바인딩
private:
	/** @brief 로딩 진행 바 (필수 바인딩) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_LoadingBar = nullptr;

	/** @brief 커스텀 로딩 배경 이미지 (선택 바인딩) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Background = nullptr;
#pragma endregion 위젯 바인딩
};
