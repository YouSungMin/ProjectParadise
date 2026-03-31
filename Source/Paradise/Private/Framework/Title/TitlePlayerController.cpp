// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Title/TitlePlayerController.h"
#include "Framework/System/GraphicsSettingsSubsystem.h"
#include "Framework/System/ParadiseCursorSubsystem.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "UI/HUD/Title/ParadiseTitleHUDWidget.h"
#include "UI/Widgets/Setting/SettingsPopupWidget.h"
#include "TimerManager.h"

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 1. 타이틀 위젯 생성 및 부착
	if (TitleHUDClass)
	{
		UUserWidget* TitleWidget = CreateWidget<UUserWidget>(this, TitleHUDClass);
		if (TitleWidget)
		{
			TitleWidget->AddToViewport();
		}
	}


	//0326 김성현 그래픽 설정 초기 체크 함수 추가
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGraphicsSettingsSubsystem* GraphicsSys = GI->GetSubsystem<UGraphicsSettingsSubsystem>())
		{
			GraphicsSys->CheckDevicePerformanceAndApply();
		}
	}

	// 2. 입력 모드 설정 (UI Only)
	// 캐릭터 이동은 막고, 마우스 커서는 보이게 설정합니다.
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);
	bShowMouseCursor = false;
	// 마우스 커서 서브시스템 커서 초기화
	if (UParadiseCursorSubsystem* CursorSys = GetGameInstance()->GetSubsystem<UParadiseCursorSubsystem>())
	{
		CachedCursorSubsystem = CursorSys;
		CursorSys->InitializeCursor(CursorWidgetClass, Tex_CustomCursor, this);
		CursorSys->ShowCursor(true);
	}
}

void ATitlePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (IA_OpenSettings)
		{
			EnhancedInputComponent->BindAction(IA_OpenSettings, ETriggerEvent::Started, this, &ATitlePlayerController::OnInputOpenSettings);
		}
	}
}

void ATitlePlayerController::OnInputOpenSettings(const FInputActionValue& Value)
{
	if (bIsTogglingSettings) return;
	bIsTogglingSettings = true;

	// 캐싱해 둔 타이틀 HUD가 있는지 확인
	if (!CachedTitleHUD)
	{
		bIsTogglingSettings = false;
		return;
	}

	// 타이틀 HUD를 통해 팝업 인스턴스를 가져옵니다 (TitleHUD에 GetSettingsPopupInstance 함수가 있어야 함!)
	USettingsPopupWidget* SettingsPopup = CachedTitleHUD->GetSettingsPopupInstance();
	if (!SettingsPopup)
	{
		bIsTogglingSettings = false;
		return;
	}

	// 열려있는지 상태 판별
	const bool bIsOpen = SettingsPopup->GetVisibility() == ESlateVisibility::Visible;

	if (bIsOpen)
	{
		if (CachedCursorSubsystem.IsValid()) CachedCursorSubsystem->ShowCursor(false);

		// 🌟 인게임/로비와 동일하게 0.1초 딜레이를 두고 닫는 함수 호출
		SettingsPopup->OnResumeGameClicked();
	}
	else
	{
		if (CachedCursorSubsystem.IsValid()) CachedCursorSubsystem->ShowCursor(true);
		SettingsPopup->OpenSettings();
	}

	// 타이머를 통해 토글 플래그 해제
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() { bIsTogglingSettings = false; });
	}
}