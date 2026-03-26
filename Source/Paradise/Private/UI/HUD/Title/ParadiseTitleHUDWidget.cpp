// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Title/ParadiseTitleHUDWidget.h"
#include "UI/Widgets/Setting/SettingsPopupWidget.h"

#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/LevelLoadingSubsystem.h"

#include "TimerManager.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Data/Assets/ParadiseFXAudioData.h"
#include "Framework/System/AudioManagementSubsystem.h"

void UParadiseTitleHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsLoadingStarted = false;

	// 1. 버튼 이벤트 바인딩
	if (Btn_ScreenTouch)
	{
		Btn_ScreenTouch->OnClicked.AddUniqueDynamic(this, &UParadiseTitleHUDWidget::OnScreenTouched);
	}
	// 2. 종료 버튼 이벤트 바인딩
	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.AddUniqueDynamic(this, &UParadiseTitleHUDWidget::OnQuitButtonClicked);
	}
	// 3. 설정 버튼 이벤트 바인딩
	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.AddUniqueDynamic(this, &UParadiseTitleHUDWidget::OnSettingsButtonClicked);
	}
	// 4. 깜빡임 애니메이션 재생 (무한 반복)
	if (Anim_BlinkText)
	{
		PlayAnimation(Anim_BlinkText, 0.0f, 0);
	}

	/** @section 설정 팝업 사전 생성 (캐싱) */
	if (SettingsPopupClass && !SettingsPopupInstance)
	{
		SettingsPopupInstance = CreateWidget<USettingsPopupWidget>(GetOwningPlayer(), SettingsPopupClass);
		if (SettingsPopupInstance)
		{
			SettingsPopupInstance->AddToViewport(100); // 팝업이 최상단에 뜨도록 ZOrder 100 부여
			SettingsPopupInstance->SetVisibility(ESlateVisibility::Collapsed); // 처음엔 숨겨둠
		}
	}
	/** @section BGM 재생 요청 */
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->BGM_Title)
		{
			if (UAudioManagementSubsystem* AudioMag = GI->GetSubsystem<UAudioManagementSubsystem>())
			{
				AudioMag->PlayBGM(GI->GlobalAudioData->BGM_Title);
			}
		}
	}
}

void UParadiseTitleHUDWidget::OnScreenTouched()
{
	// 중복 실행 방지
	if (bIsLoadingStarted) return;
	bIsLoadingStarted = true;

	// 터치 시 애니메이션 멈춤 or 속도 증가 등의 피드백 연출 가능
	// StopAnimation(Anim_BlinkText);

	// 스크린 터치 진입 효과음 재생
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_TouchToStart)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_TouchToStart);
		}

		if (UAudioManagementSubsystem* AudioMag = GI->GetSubsystem<UAudioManagementSubsystem>())
		{
			AudioMag->StopBGM(1.0f);
		}
	}

	// BGM 정지 요청
	if (UAudioManagementSubsystem* AudioMag = GetGameInstance()->GetSubsystem<UAudioManagementSubsystem>())
	{
		AudioMag->StopBGM(1.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("[타이틀] 스크린 터치 및 클릭 -> Request Lobby Load"));

	// 2. 타이머를 설정하여 1초(페이드 아웃 및 효과음 재생 시간) 뒤에 로딩 맵으로 넘깁니다.
	FTimerHandle TransitionTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TransitionTimerHandle,
		this,
		&UParadiseTitleHUDWidget::ExecuteLevelTransition,
		0.5f, // 1.0초 대기 (기획에 따라 0.5f 등으로 조절 가능)
		false
	);
}

void UParadiseTitleHUDWidget::OnQuitButtonClicked()
{
	// PC 빌드 등에서 게임을 완전히 종료합니다.
	UE_LOG(LogTemp, Log, TEXT("[타이틀] 게임 종료 요청"));
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UParadiseTitleHUDWidget::OnSettingsButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[타이틀] 설정 버튼 클릭"));

	// 공통 설정 팝업 버튼 클릭음 재생
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_SettingsOpen)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_SettingsOpen);
		}
	}

	if (SettingsPopupInstance)
	{
		SettingsPopupInstance->OpenSettings();
	}
}

void UParadiseTitleHUDWidget::ExecuteLevelTransition()
{
	// 1. 서브시스템을 찾는다
	if (ULevelLoadingSubsystem* LoadingSystem = GetGameInstance()->GetSubsystem<ULevelLoadingSubsystem>())
	{
		// 2. 서브시스템에게 요청한다 (중간 로딩맵 이름 "L_Loading" 명시)
		LoadingSystem->StartLevelTransition(NextLevelName, FName("L_Loading"), PreloadAssets);
	}
	else
	{
		// Fallback: GI 캐스팅 실패 시 일반 로딩
		UE_LOG(LogTemp, Warning, TEXT("[타이틀] ParadiseGameInstance Not Found! Using OpenLevel directly."));
		UGameplayStatics::OpenLevel(this, NextLevelName);
	}
}
