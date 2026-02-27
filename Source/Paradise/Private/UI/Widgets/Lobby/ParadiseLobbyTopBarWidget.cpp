// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/ParadiseLobbyTopBarWidget.h"
#include "UI/Widgets/Setting/SettingsPopupWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h" // 종료 기능을 위해 필요
#include "Framework/System/EconomySubsystem.h"

void UParadiseLobbyTopBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 버튼 이벤트 바인딩
	if (Btn_Settings) Btn_Settings->OnClicked.AddDynamic(this, &UParadiseLobbyTopBarWidget::OnSettingsClicked);
	if (Btn_QuitGame) Btn_QuitGame->OnClicked.AddDynamic(this, &UParadiseLobbyTopBarWidget::OnQuitGameClicked);

	// 2. 설정 팝업 사전 생성 및 캐싱
	if (SettingsPopupClass && !SettingsPopupInstance)
	{
		SettingsPopupInstance = CreateWidget<USettingsPopupWidget>(GetOwningPlayer(), SettingsPopupClass);
		if (SettingsPopupInstance)
		{
			SettingsPopupInstance->AddToViewport(100);
			SettingsPopupInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 3. 경제 서브시스템 연동 (초기 데이터 로드 및 델리게이트 구독)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>())
		{
			// 이벤트 구독
			EconSys->OnCurrencyChanged.AddDynamic(this, &UParadiseLobbyTopBarWidget::HandleCurrencyChanged);

			// 현재 지갑 상태를 읽어와서 최초 1회 화면 갱신
			int32 CurrentGold = EconSys->GetCurrency(ECurrencyType::Gold);
			int32 CurrentAether = EconSys->GetCurrency(ECurrencyType::Aether);
			UpdateCurrencyUI(CurrentGold, CurrentAether);
		}
	}
	
	// 서브시스템에서 받아서 옴
	UpdateCurrencyUI(0, 0);
}

void UParadiseLobbyTopBarWidget::NativeDestruct()
{
	if (Btn_Settings) Btn_Settings->OnClicked.RemoveAll(this);
	if (Btn_QuitGame) Btn_QuitGame->OnClicked.RemoveAll(this);

	// 메모리 누수 방지: 델리게이트 구독 해제
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>())
		{
			EconSys->OnCurrencyChanged.RemoveDynamic(this, &UParadiseLobbyTopBarWidget::HandleCurrencyChanged);
		}
	}

	Super::NativeDestruct();
}

#pragma region 로직 구현
void UParadiseLobbyTopBarWidget::UpdateCurrencyUI(int32 InGold, int32 InAether)
{
	UpdateGoldText(InGold);
	UpdateAetherText(InAether);
}

void UParadiseLobbyTopBarWidget::UpdateGoldText(int32 Amount)
{
	if (!Text_GoldAmount) return;

	// 3자리마다 콤마(,)를 찍는 서식 적용 (ex: 1,000,000)
	FNumberFormattingOptions NumberFormat;
	NumberFormat.UseGrouping = true;
	Text_GoldAmount->SetText(FText::AsNumber(Amount, &NumberFormat));
}

void UParadiseLobbyTopBarWidget::UpdateAetherText(int32 Amount)
{
	if (!Text_AetherAmount) return;

	// 에테르도 골드처럼 콤마를 찍고 싶다면 동일하게 적용 가능
	FNumberFormattingOptions NumberFormat;
	NumberFormat.UseGrouping = true;
	Text_AetherAmount->SetText(FText::AsNumber(Amount, &NumberFormat));
}

void UParadiseLobbyTopBarWidget::HandleCurrencyChanged(ECurrencyType CurrencyType, int32 OldAmount, int32 NewAmount)
{
	// [최적화] 변경된 재화의 종류를 판별하여 필요한 텍스트 위젯만 갱신합니다.
	if (CurrencyType == ECurrencyType::Gold)
	{
		UpdateGoldText(NewAmount);
	}
	else if (CurrencyType == ECurrencyType::Aether)
	{
		UpdateAetherText(NewAmount);
	}
}

void UParadiseLobbyTopBarWidget::OnSettingsClicked()
{
	/** @section 팝업 열기 */
	if (SettingsPopupInstance)
	{
		SettingsPopupInstance->OpenSettings();
	}
}

void UParadiseLobbyTopBarWidget::OnQuitGameClicked()
{
	// 에디터에서는 시뮬레이션 종료, 빌드에서는 게임 종료
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, true);
	}
}
#pragma endregion 로직 구현
