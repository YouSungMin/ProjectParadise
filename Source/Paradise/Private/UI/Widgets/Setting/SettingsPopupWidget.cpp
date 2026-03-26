// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Setting/SettingsPopupWidget.h"
#include "Components/Slider.h"
#include "UI/Widgets/Ingame/ParadiseCommonButton.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Framework/System/AudioSettingsSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Assets/ParadiseFXAudioData.h"

#pragma region 생명주기
void USettingsPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true); // 키보드/마우스 입력을 온전히 받을 수 있도록 설정

	/** @section 1. AudioSettingsSubsystem 캐싱 */
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedAudioSettings = GI->GetSubsystem<UAudioSettingsSubsystem>();
		if (!CachedAudioSettings.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[SettingsPopup] AudioSettingsSubsystem을 찾을 수 없습니다."));
		}
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

	/** @section 2. 게임 일시정지 및 조작 권한 탈취 */
	if (APlayerController* PC = GetOwningPlayer())
	{
		// 기획자가 true로 체크했을 때만 시간을 멈춥니다!
		if (bPauseGameOnOpen)
		{
			PC->SetPause(true);
		}

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void USettingsPopupWidget::CloseSettings()
{
	/** @section 1. 가시성 끄기 (파괴하지 않고 숨김 처리) */
	SetVisibility(ESlateVisibility::Collapsed);

	/** @section 2. 디스크에 1회 저장 (I/O 병목 회피) */
	if (CachedAudioSettings.IsValid())
	{
		CachedAudioSettings->SaveToSlot();
		UE_LOG(LogTemp, Log, TEXT("[SettingsPopup] 팝업 닫힘 → 볼륨 디스크 저장 완료"));
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
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직
void USettingsPopupWidget::InitializeVolumeSliders()
{
	if (!CachedAudioSettings.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SettingsPopup] AudioSettings가 유효하지 않아 슬라이더 초기화를 건너뜁니다."));
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

	UE_LOG(LogTemp, Log, TEXT("[SettingsPopup] 슬라이더 초기화: BGM=%.2f, SFX=%.2f"), LoadedBGMVolume, LoadedSFXVolume);
}

void USettingsPopupWidget::OnBGMVolumeChanged(float Value)
{
	/** @section 1. SetSoundMixClassOverride로 즉시 적용 */
	if (MasterSoundMix && BGMSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this,
			MasterSoundMix,
			BGMSoundClass,
			Value,
			1.0f,  // Pitch (피치 변경 안 함)
			0.0f,  // FadeInTime (즉시 적용)
			true   // bApplyToChildren (자식 사운드 클래스에도 적용)
		);
	}

	/** @section 2. 서브시스템의 RAM만 변경 (디스크 저장 안 함!) */
	if (CachedAudioSettings.IsValid())
	{
		CachedAudioSettings->SetBGMVolume(Value);
	}
}

void USettingsPopupWidget::OnSFXVolumeChanged(float Value)
{
	/** @section 1. SetSoundMixClassOverride로 즉시 적용 */
	if (MasterSoundMix && SFXSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this,
			MasterSoundMix,
			SFXSoundClass,
			Value,
			1.0f,
			0.0f,
			true
		);
	}

	/** @section 2. 서브시스템의 RAM만 변경 (디스크 저장 안 함!) */
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

	CloseSettings();
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
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (bPauseGameOnOpen)
		{
			PC->SetPause(false);
		}
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
#pragma endregion 내부 로직