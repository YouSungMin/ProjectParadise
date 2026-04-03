// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Title/TitlePlayerController.h"
#include "Framework/System/GraphicsSettingsSubsystem.h"

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
			CachedTitleHUD = Cast<UParadiseTitleHUDWidget>(TitleWidget);
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
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
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

	// 컨트롤러는 그저 HUD에게 "팝업 토글해!" 라고 지시만 합니다.
	CachedTitleHUD->ToggleSettingsPopup();

	bIsTogglingSettings = false;
}