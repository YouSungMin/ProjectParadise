// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Summon/ParadiseSummonPopup.h"
#include "UI/Panel/Summon/ParadiseSummonPanel.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/System/EconomySubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"

#pragma region 생명주기
void UParadiseSummonPopup::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 이벤트 바인딩
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.AddDynamic(this, &UParadiseSummonPopup::OnCharacterTabClicked);
	if (Btn_Tab_Equipment) Btn_Tab_Equipment->OnClicked.AddDynamic(this, &UParadiseSummonPopup::OnEquipmentTabClicked);
	if (Btn_Back) Btn_Back->OnClicked.AddDynamic(this, &UParadiseSummonPopup::OnBackButtonClicked);

	// 2. 컨트롤러 및 서브시스템 캐싱 (최적화)
	CachedPlayerController = GetOwningPlayer<ALobbyPlayerController>();
	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());

	if (CachedGI.IsValid())
	{
		CachedEconomySubsystem = CachedGI->GetSubsystem<UEconomySubsystem>();

		if (CachedEconomySubsystem.IsValid())
		{
			CachedEconomySubsystem->OnCurrencyChanged.RemoveDynamic(this, &UParadiseSummonPopup::HandleCurrencyChanged);
			CachedEconomySubsystem->OnCurrencyChanged.AddDynamic(this, &UParadiseSummonPopup::HandleCurrencyChanged);
		}
	}

	// 3. 재화 UI 최초 갱신
	RefreshCurrencyUI();

	// 4. 초기 상태 설정 (캐릭터 탭 기본)
	SwitchTab(INDEX_CHARACTER);
}

void UParadiseSummonPopup::NativeDestruct()
{
	// 델리게이트 안전 해제
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.RemoveAll(this);
	if (Btn_Tab_Equipment) Btn_Tab_Equipment->OnClicked.RemoveAll(this);
	if (Btn_Back) Btn_Back->OnClicked.RemoveAll(this);

	if (CachedEconomySubsystem.IsValid())
	{
		CachedEconomySubsystem->OnCurrencyChanged.RemoveDynamic(this, &UParadiseSummonPopup::HandleCurrencyChanged);
	}

	CachedEconomySubsystem = nullptr;
	CachedPlayerController = nullptr;
	CachedGI = nullptr;

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스
void UParadiseSummonPopup::RefreshCurrencyUI()
{
	if (Text_AetherAmount && CachedEconomySubsystem.IsValid())
	{
		const int32 CurrentEther = CachedEconomySubsystem->GetCurrency(ECurrencyType::Aether);

		FNumberFormattingOptions NumberFormat;
		NumberFormat.UseGrouping = true; // 3자리 콤마
		Text_AetherAmount->SetText(FText::AsNumber(CurrentEther, &NumberFormat));
	}
}
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
void UParadiseSummonPopup::OnCharacterTabClicked()
{
	SwitchTab(INDEX_CHARACTER);
}

void UParadiseSummonPopup::OnEquipmentTabClicked()
{
	SwitchTab(INDEX_EQUIPMENT);
}

void UParadiseSummonPopup::OnBackButtonClicked()
{
	// 1. 상태 저장 (Model 갱신)
	if (CachedGI.IsValid())
	{
		CachedGI->SaveGameData();
	}

	// 2. 화면 복귀 요청 (기존의 무거운 탐색을 지우고 캐싱된 컨트롤러 사용 - SRP)
	if (CachedPlayerController.IsValid())
	{
		CachedPlayerController->MoveCameraToMenu(EParadiseLobbyMenu::None);
	}
}

void UParadiseSummonPopup::SwitchTab(int32 NewIndex)
{
	if (!Switcher_Content) return;

	// 1. 위젯 스위처 인덱스 변경
	Switcher_Content->SetActiveWidgetIndex(NewIndex);

	// 2. 해당 패널 데이터 갱신 (선택되었을 때만 데이터 로드 -> 최적화)
	if (NewIndex == INDEX_CHARACTER && Panel_Character)
	{
		Panel_Character->RefreshPanelData();
	}
	else if (NewIndex == INDEX_EQUIPMENT && Panel_Equipment)
	{
		Panel_Equipment->RefreshPanelData();
	}

	// 3. 버튼 스타일 업데이트 (선택된 탭 비활성화 등 시각적 피드백)
	if (Btn_Tab_Character) Btn_Tab_Character->SetIsEnabled(NewIndex != INDEX_CHARACTER);
	if (Btn_Tab_Equipment) Btn_Tab_Equipment->SetIsEnabled(NewIndex != INDEX_EQUIPMENT);
}

void UParadiseSummonPopup::HandleCurrencyChanged(ECurrencyType CurrencyType, int32 OldAmount, int32 NewAmount)
{
	// 현재 이 팝업창은 '에테르(Aether)'만 표시하고 있으므로, 
	// 골드나 다른 재화가 변했을 때는 무시하고 에테르가 변했을 때만 텍스트를 갱신합니다. (최적화)
	if (CurrencyType == ECurrencyType::Aether)
	{
		RefreshCurrencyUI();
	}
}
#pragma endregion 내부 로직