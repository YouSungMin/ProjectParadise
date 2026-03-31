// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Title/TitlePlayerController.h"
#include "Framework/System/GraphicsSettingsSubsystem.h"
#include "Framework/System/ParadiseCursorSubsystem.h"
#include "Blueprint/UserWidget.h"

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

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
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	SetInputMode(InputMode);
	bShowMouseCursor = false;
	// 마우스 커서 서브시스템 커서 초기화
	if (UParadiseCursorSubsystem* CursorSys = GetGameInstance()->GetSubsystem<UParadiseCursorSubsystem>())
	{
		CursorSys->InitializeCursor(CursorWidgetClass, Tex_CustomCursor, this);
		CursorSys->ShowCursor(true);
	}
}
