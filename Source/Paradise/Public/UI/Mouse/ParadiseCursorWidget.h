// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseCursorWidget.generated.h"

#pragma region 전방 선언
class UImage;
class UTexture2D;
class UCanvasPanelSlot;
#pragma endregion 전방 선언
/**
 * @class UInGameCursorWidget
 * @brief 커스텀 마우스 커서를 표시하는 소프트웨어 커서 위젯입니다.
 * @details OS 기본 커서를 숨기고 이 위젯으로 대체합니다.
 *          마우스 이동 시 HUD에서 위치를 갱신합니다.
 */
UCLASS()
class PARADISE_API UParadiseCursorWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
    virtual void NativeConstruct() override;

#pragma region 외부 인터페이스
public:
    /**
     * @brief 커서 텍스처를 설정합니다.
     * @param InTexture 표시할 커서 텍스처
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Cursor")
    void SetCursorTexture(UTexture2D* InTexture);

    /**
     * @brief 커서 위젯의 화면 위치를 갱신합니다.
     * @param InPosition 화면 좌표 (픽셀)
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Cursor")
    void UpdatePosition(FVector2D InPosition);
#pragma endregion 외부 인터페이스

#pragma region 위젯 바인딩
private:
    /** @brief 커서 이미지 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Img_Cursor = nullptr;
#pragma endregion 위젯 바인딩
};
