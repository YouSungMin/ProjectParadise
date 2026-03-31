// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/ParadiseCursorSubsystem.h"
#include "UI/Mouse/ParadiseCursorWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"

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
    if (IsMobilePlatform()) return;
    if (!InWidgetClass || !InPlayerController) return;

    CleanupCursor();

    CachedPlayerController = InPlayerController;
    // OS 기본 커서 숨김
    InPlayerController->bShowMouseCursor = false;
    // PIE 뷰포트 포커스 강제 획득
    FSlateApplication::Get().SetAllUserFocusToGameViewport();

    CursorWidgetInstance = CreateWidget<UParadiseCursorWidget>(InPlayerController, InWidgetClass);
    if (!CursorWidgetInstance) return;

    CursorWidgetInstance->AddToViewport(99999);

    if (InCursorTexture)
    {
        CursorWidgetInstance->SetCursorTexture(InCursorTexture);
    }

    // 앵커 오프셋을 없애기 위해 무조건 (0,0)에 고정시키고 RenderTranslation으로만 움직입니다.
    CursorWidgetInstance->SetAlignmentInViewport(FVector2D::ZeroVector);
    CursorWidgetInstance->SetPositionInViewport(FVector2D::ZeroVector, false);
    CursorWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);

    // 고정 타이머 대신 프레임 레이트와 완벽히 동기화된 갱신 시작 (지연 없는 마우스 속도 보장)
    CursorTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UParadiseCursorSubsystem::UpdateCursorTick)
    );
}


void UParadiseCursorSubsystem::ShowCursor(bool bShow)
{
    if (IsMobilePlatform() || !CursorWidgetInstance) return;

    // 메뉴가 열려서 강제로 보여야 하는 상태일 때는 숨김(false) 명령을 무시합니다.
    bCursorHidden = !bShow;
    CursorWidgetInstance->SetVisibility(
        bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UParadiseCursorSubsystem::SetCursorForceVisible(bool bForce)
{
    bCursorHidden = bForce;

    // bForce=true면 강제 표시(숨김 아님), bForce=false면 일반 로직 복귀
    if (bForce)
    {
        bCursorHidden = false;
        ShowCursor(true);
    }
    else
    {
        bCursorHidden = false;
    }
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
    if (CursorTickHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(CursorTickHandle);
        CursorTickHandle.Reset();
    }

    if (CursorWidgetInstance)
    {
        CursorWidgetInstance->RemoveFromParent();
        CursorWidgetInstance = nullptr;
    }

    CachedPlayerController = nullptr;
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직 구현
bool UParadiseCursorSubsystem::UpdateCursorTick(float DeltaTime)
{
    // 위젯이 파괴되었거나 초기화 해제 시 루프 자동 정지
    if (!CursorWidgetInstance) return false;

    FVector2D CursorPos = FVector2D::ZeroVector;
    if (!GetCursorPositionInViewportSpace(CursorPos)) return true;

    // 마우스가 움직였고 커서가 숨겨진 상태라면 자동으로 표시
    if (bCursorHidden && !CursorPos.Equals(LastCursorPos, 1.0f))
    {
        ShowCursor(true);
    }

    LastCursorPos = CursorPos;
    CursorWidgetInstance->SetRenderTranslation(CursorPos);
    return true;
}

bool UParadiseCursorSubsystem::GetCursorPositionInViewportSpace(FVector2D& OutPosition) const
{
    if (UWorld* World = GetGameInstance()->GetWorld())
    {
        // GetMousePositionOnViewport 사용
        // 화면 해상도나 창 모드 전환 시에도 DPI 스케일을 완벽하게 보정하여 실제 커서 위치와 UI 좌표를 정확히 일치시킵니다.
        OutPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(World);
        return true;
    }
    return false;
}
#pragma endregion 내부 로직 구현