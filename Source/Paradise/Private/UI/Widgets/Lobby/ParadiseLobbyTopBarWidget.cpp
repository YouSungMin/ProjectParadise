// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/ParadiseLobbyTopBarWidget.h"
#include "UI/Widgets/Setting/SettingsPopupWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h" // 종료 기능을 위해 필요

void UParadiseLobbyTopBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 버튼 이벤트 바인딩
	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.AddDynamic(this, &UParadiseLobbyTopBarWidget::OnSettingsClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TopBar] Btn_Settings가 바인딩되지 않았습니다."));
	}

	if (Btn_QuitGame)
	{
		Btn_QuitGame->OnClicked.AddDynamic(this, &UParadiseLobbyTopBarWidget::OnQuitGameClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TopBar] Btn_QuitGame이 바인딩되지 않았습니다."));
	}

	/** @section 설정 팝업 사전 생성 (캐싱) */
	if (SettingsPopupClass && !SettingsPopupInstance)
	{
		SettingsPopupInstance = CreateWidget<USettingsPopupWidget>(GetOwningPlayer(), SettingsPopupClass);
		if (SettingsPopupInstance)
		{
			SettingsPopupInstance->AddToViewport(100);
			SettingsPopupInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 2. 초기화 시 임시 데이터로 갱신 (테스트용)
	// 실제 게임에서는 PlayerState나 GameInstance에서 데이터를 받아와야 합니다.
	UpdateCurrencyUI(0, 0);
}

#pragma region 로직 구현
void UParadiseLobbyTopBarWidget::UpdateCurrencyUI(int32 InGold, int32 InEther)
{
	// 골드 텍스트 갱신
	if (Text_GoldAmount)
	{
		// 3자리마다 콤마(,)를 찍는 서식 적용 (ex: 1,000,000)
		FNumberFormattingOptions NumberFormat;
		NumberFormat.UseGrouping = true;

		Text_GoldAmount->SetText(FText::AsNumber(InGold, &NumberFormat));
	}

	// 에테르 텍스트 갱신
	if (Text_AetherAmount)
	{
		Text_AetherAmount->SetText(FText::AsNumber(InEther));
	}
}

void UParadiseLobbyTopBarWidget::OnSettingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[TopBar] 설정 버튼 클릭됨 (기능 구현 예정)"));

	/** @section 팝업 열기 */
	if (SettingsPopupInstance)
	{
		SettingsPopupInstance->OpenSettings();
	}
}

void UParadiseLobbyTopBarWidget::OnQuitGameClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[TopBar] 게임 종료 버튼 클릭됨"));

	// 에디터에서는 시뮬레이션 종료, 빌드에서는 게임 종료
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, true);
	}
}
#pragma endregion 로직 구현
