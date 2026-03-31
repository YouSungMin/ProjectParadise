// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Setting/SettingsPopupWidget.h"
#include "Components/Slider.h"
#include "UI/Widgets/Ingame/ParadiseCommonButton.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Framework/System/AudioSettingsSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/GraphicsSettingsSubsystem.h"
#include "Framework/System/ParadiseCursorSubsystem.h"
#include "Components/TextBlock.h"
#include "Data/Assets/ParadiseFXAudioData.h"

#pragma region 생명주기
void USettingsPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true); // 키보드/마우스 입력을 온전히 받을 수 있도록 설정

	/** 1. AudioSettingsSubsystem과 GraphicsSettingsSubsystem 캐싱 */
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedAudioSettings = GI->GetSubsystem<UAudioSettingsSubsystem>();
		CachedGraphicsSettings = GI->GetSubsystem<UGraphicsSettingsSubsystem>();
	}

	// 그래픽 슬라이더 바인딩 및 초기값 세팅
	if (Slider_Graphics)
	{
		Slider_Graphics->SetMinValue(0.0f);
		Slider_Graphics->SetMaxValue(1.0f);
		Slider_Graphics->SetStepSize(0.333f); // 4단계 스냅

		// 현재 저장된 퀄리티로 초기화
		if (CachedGraphicsSettings.IsValid())
		{
			int32 CurrentQuality = CachedGraphicsSettings->GetGraphicsQuality();
			Slider_Graphics->SetValue(CurrentQuality / 3.0f);

			if (Text_GraphicsQuality)
			{
				Text_GraphicsQuality->SetText(GetQualityText(CurrentQuality));
			}
		}

		Slider_Graphics->OnValueChanged.AddDynamic(this, &USettingsPopupWidget::OnGraphicsQualityChanged);
	}

	/** @section 2. 슬라이더 델리게이트 바인딩 */
	if (Slider_BGM)
	{
		Slider_BGM->OnValueChanged.AddDynamic(this, &USettingsPopupWidget::OnBGMVolumeChanged);
	}
	if (Slider_SFX)
	{
		Slider_SFX->OnValueChanged.AddDynamic(this, &USettingsPopupWidget::OnSFXVolumeChanged);
	}

	/** @section 3. 버튼 델리게이트 바인딩 */
	if (Btn_ResumeGame)
	{
		Btn_ResumeGame->OnClicked().AddUObject(this, &USettingsPopupWidget::OnResumeGameClicked);

		Btn_ResumeGame->SetButtonText(FText::GetEmpty());
		Btn_ResumeGame->SetButtonIcon(Tex_ResumeGame);
	}
	if (Btn_ReturnToLobby)
	{
		Btn_ReturnToLobby->OnClicked().AddUObject(this, &USettingsPopupWidget::OnReturnToLobbyClicked);

		Btn_ReturnToLobby->SetButtonText(FText::GetEmpty());
		Btn_ReturnToLobby->SetButtonIcon(Tex_ReturnToLobby);
	}
	if (Btn_Retry)
	{
		Btn_Retry->OnClicked().AddUObject(this, &USettingsPopupWidget::OnRetryClicked);
		Btn_Retry->SetButtonText(FText::GetEmpty());
		Btn_Retry->SetButtonIcon(Tex_Retry);
	}

	/** @section 4. 슬라이더 초기값 세팅 (서브시스템의 RAM 볼륨) */
	InitializeVolumeSliders();
}

#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void USettingsPopupWidget::OpenSettings()
{
	/** @section 1. 가시성 켜기 */
	SetVisibility(ESlateVisibility::Visible);

	if (UParadiseCursorSubsystem* CursorSys = GetGameInstance()->GetSubsystem<UParadiseCursorSubsystem>())
	{
		CursorSys->SetCursorForceVisible(true);
	}

	/** @section 2. 게임 일시정지 및 조작 권한 탈취 */
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (bPauseGameOnOpen)
		{
			PC->SetPause(true);
		}

		FInputModeUIOnly InputMode;
		//InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void USettingsPopupWidget::CloseSettings()
{
	/** @section 1. 가시성 끄기 (파괴하지 않고 숨김 처리) */
	SetVisibility(ESlateVisibility::Collapsed);

	// 팝업이 닫힐 때 커서 강제 표시 해제 (인게임 원래 로직으로 복귀)
	if (UParadiseCursorSubsystem* CursorSys = GetGameInstance()->GetSubsystem<UParadiseCursorSubsystem>())
	{
		CursorSys->SetCursorForceVisible(false);
	}

	/** @section 2. 디스크에 1회 저장 (I/O 병목 회피) */
	if (CachedAudioSettings.IsValid())
	{
		CachedAudioSettings->SaveToSlot();
		CachedAudioSettings->ApplyVolumeSettings();
		//UE_LOG(LogTemp, Log, TEXT("[SettingsPopup] 팝업 닫힘 → 볼륨 디스크 저장 완료"));
	}
	/** @section 3. 게임 시간 복구 및 입력 모드 반환 */
	if (APlayerController* PC = GetOwningPlayer())
	{
		// 기획자가 true로 체크했을 때만 시간을 다시 풉니다!
		if (bPauseGameOnOpen)
		{
			PC->SetPause(false);
		}

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직
void USettingsPopupWidget::InitializeVolumeSliders()
{
	if (!CachedAudioSettings.IsValid())
	{
		//UE_LOG(LogTemp, Warning, TEXT("[SettingsPopup] AudioSettings가 유효하지 않아 슬라이더 초기화를 건너뜁니다."));
		return;
	}

	/** @section 서브시스템의 RAM 볼륨을 슬라이더에 반영 */
	const float LoadedBGMVolume = CachedAudioSettings->GetBGMVolume();
	const float LoadedSFXVolume = CachedAudioSettings->GetSFXVolume();

	if (Slider_BGM)
	{
		Slider_BGM->SetValue(LoadedBGMVolume);
	}
	if (Slider_SFX)
	{
		Slider_SFX->SetValue(LoadedSFXVolume);
	}

	//UE_LOG(LogTemp, Log, TEXT("[SettingsPopup] 슬라이더 초기화: BGM=%.2f, SFX=%.2f"), LoadedBGMVolume, LoadedSFXVolume);
}

void USettingsPopupWidget::OnBGMVolumeChanged(float Value)
{
	if (CachedAudioSettings.IsValid())
	{
		CachedAudioSettings->SetBGMVolume(Value);
	}
}

void USettingsPopupWidget::OnSFXVolumeChanged(float Value)
{
	if (CachedAudioSettings.IsValid())
	{
		CachedAudioSettings->SetSFXVolume(Value);
	}
}

void USettingsPopupWidget::OnResumeGameClicked()
{
	// 공통 뒤로가기 버튼 효과음 재생
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_CommonBack)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_CommonBack);
		}
	}
	// 2. 🌟 핵심: 게임의 일시정지(Pause)만 먼저 풀어줍니다! (그래야 아래의 GetWorld() 타이머가 돌아갑니다)
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (bPauseGameOnOpen)
		{
			PC->SetPause(false);
		}
	}

	// 3. 🌟 핵심: 버튼이 '눌렸다 떼어지는' Slate 이벤트 처리가 완전히 끝날 수 있도록 딱 0.1초의 시간을 벌어주고 CloseSettings를 실행합니다!
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_Resume,
			this,
			&USettingsPopupWidget::CloseSettings, // 기존 함수 손댈 필요 없이 그대로 호출!
			0.1f,
			false
		);
	}
}

void USettingsPopupWidget::OnReturnToLobbyClicked()
{
	// 여기도 뒤로가기/복귀 뉘앙스이므로 동일하게 처리 가능
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_CommonBack)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_CommonBack);
		}
	}

	ResumeTimeOnly();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_ReturnToLobby,
			this,
			&USettingsPopupWidget::ExecuteReturnToLobby,
			0.3f,
			false
		);
	}

}

void USettingsPopupWidget::OnRetryClicked()
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_IngameRetry)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_IngameRetry);
		}
	}

	ResumeTimeOnly();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_Retry,
			this,
			&USettingsPopupWidget::ExecuteRetry,
			0.3f,
			false
		);
	}
}

void USettingsPopupWidget::ResumeTimeOnly()
{
	// 볼륨 디스크 저장
	if (CachedAudioSettings.IsValid())
	{
		CachedAudioSettings->SaveToSlot();
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (bPauseGameOnOpen)
		{
			PC->SetPause(false);
		}
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void USettingsPopupWidget::ExecuteReturnToLobby()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULevelLoadingSubsystem* LoadingSys = GI->GetSubsystem<ULevelLoadingSubsystem>())
		{
			TArray<TSoftObjectPtr<UObject>> EmptyAssets;
			LoadingSys->StartLevelTransition(LobbyLevelName, LoadingMapName, EmptyAssets);
		}
		else
		{
			UGameplayStatics::OpenLevel(this, LobbyLevelName);
		}
	}
}

void USettingsPopupWidget::ExecuteRetry()
{
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
}

void USettingsPopupWidget::OnGraphicsQualityChanged(float Value)
{
	// 0.0~1.0 → 0~3 변환 (스냅)
	const int32 Quality = FMath::Clamp(FMath::RoundToInt(Value * 3.0f), 0, 3);

	if (CachedGraphicsSettings.IsValid())
	{
		CachedGraphicsSettings->SetGraphicsQuality(Quality);
	}

	if (Text_GraphicsQuality)
	{
		Text_GraphicsQuality->SetText(GetQualityText(Quality));
	}
}

FText USettingsPopupWidget::GetQualityText(int32 Quality) const
{
	switch (Quality)
	{
	case 0:  return FText::FromString(TEXT("낮음"));
	case 1:  return FText::FromString(TEXT("보통"));
	case 2:  return FText::FromString(TEXT("높음"));
	case 3:  return FText::FromString(TEXT("최상"));
	default: return FText::FromString(TEXT("보통"));
	}
}
#pragma endregion 내부 로직