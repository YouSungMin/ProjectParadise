// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/ParadiseCursorSubsystem.h"
#include "UI/Mouse/ParadiseCursorWidget.h"
#include "GameFramework/PlayerController.h"

#pragma region 생명주기
void UParadiseCursorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UParadiseCursorSubsystem::Deinitialize()
{
    CleanupCursor();
    Super::Deinitialize();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void UParadiseCursorSubsystem::InitializeCursor(
    TSubclassOf<UParadiseCursorWidget> InWidgetClass,
    UTexture2D* InCursorTexture,
    APlayerController* InPlayerController)
{
    // 모바일 플랫폼에서는 소프트웨어 커서 사용 안 함
    if (IsMobilePlatform()) return;
    if (!InWidgetClass || !InPlayerController) return;

    // 기존 커서 정리
    CleanupCursor();

    // OS 기본 커서 숨김
    InPlayerController->bShowMouseCursor = false;

    // 소프트웨어 커서 생성
    CursorWidgetInstance = CreateWidget<UParadiseCursorWidget>(InPlayerController, InWidgetClass);
    if (CursorWidgetInstance)
    {
        CursorWidgetInstance->AddToViewport(99998);
        if (InCursorTexture)
        {
            CursorWidgetInstance->SetCursorTexture(InCursorTexture);
        }
        CursorWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void UParadiseCursorSubsystem::ShowCursor(bool bShow)
{
    if (IsMobilePlatform()) return;
    if (!CursorWidgetInstance) return;

    CursorWidgetInstance->SetVisibility(
        bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UParadiseCursorSubsystem::UpdateCursorPosition(FVector2D InPosition)
{
    if (IsMobilePlatform()) return;
    if (!CursorWidgetInstance) return;

    CursorWidgetInstance->UpdatePosition(InPosition);
}

bool UParadiseCursorSubsystem::IsMobilePlatform() const
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    return true;
#else
    return false;
#endif
}

void UParadiseCursorSubsystem::CleanupCursor()
{
    if (CursorWidgetInstance)
    {
        CursorWidgetInstance->RemoveFromParent();
        CursorWidgetInstance = nullptr;
    }
}
#pragma endregion 외부 인터페이스 구현
