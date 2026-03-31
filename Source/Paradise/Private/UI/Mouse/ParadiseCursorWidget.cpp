// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Mouse/ParadiseCursorWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"

#pragma region 생명주기
void UParadiseCursorWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::HitTestInvisible);
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void UParadiseCursorWidget::SetCursorTexture(UTexture2D* InTexture)
{
    if (Img_Cursor && InTexture)
    {
        Img_Cursor->SetBrushFromTexture(InTexture);
    }
}

void UParadiseCursorWidget::UpdatePosition(FVector2D InPosition)
{
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        CanvasSlot->SetPosition(InPosition);
    }
}
#pragma endregion 외부 인터페이스 구현
