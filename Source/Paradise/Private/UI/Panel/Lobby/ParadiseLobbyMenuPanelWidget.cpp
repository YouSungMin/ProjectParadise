// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Lobby/ParadiseLobbyMenuPanelWidget.h"
#include "Components/Button.h"
#include "Framework/Lobby/LobbyPlayerController.h"

void UParadiseLobbyMenuPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 컨트롤러 캐싱 (매번 GetOwningPlayer를 호출하는 비용 절약)
	CachedController = GetOwningPlayer<ALobbyPlayerController>();
	if (!CachedController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuPanel] Owning Player Controller is NOT ALobbyPlayerController!"));
	}

	// 2. 버튼 델리게이트 바인딩 (안전하게 nullptr 체크 후 연결)
	if (Btn_Battle) Btn_Battle->OnClicked.AddDynamic(this, &UParadiseLobbyMenuPanelWidget::OnClickBattle);
	if (Btn_Summon) Btn_Summon->OnClicked.AddDynamic(this, &UParadiseLobbyMenuPanelWidget::OnClickSummon);
	if (Btn_Squad) Btn_Squad->OnClicked.AddDynamic(this, &UParadiseLobbyMenuPanelWidget::OnClickSquad);
	if (Btn_Enhance) Btn_Enhance->OnClicked.AddDynamic(this, &UParadiseLobbyMenuPanelWidget::OnClickEnhance);
	if (Btn_Codex) Btn_Codex->OnClicked.AddDynamic(this, &UParadiseLobbyMenuPanelWidget::OnClickCodex);
}

#pragma region 로직 구현 (Logic Implementation)

void UParadiseLobbyMenuPanelWidget::RequestMenuChange(EParadiseLobbyMenu InMenu)
{
	if (CachedController)
	{
		// [변경] 바로 SetLobbyMenu 하지 않고, 카메라 이동 요청!
		// (단, Battle처럼 카메라 이동이 필요한 메뉴만. 나머진 바로 띄워도 됨)
		if (InMenu == EParadiseLobbyMenu::Battle || InMenu == EParadiseLobbyMenu::Summon)
		{
			CachedController->MoveCameraToMenu(InMenu);
		}
		else
		{
			// 소환이나 인벤토리는 그냥 팝업으로 띄운다면 기존대로 유지
			CachedController->SetLobbyMenu(InMenu);
		}
	}
}

void UParadiseLobbyMenuPanelWidget::OnClickBattle()
{
	RequestMenuChange(EParadiseLobbyMenu::Battle);
}	

void UParadiseLobbyMenuPanelWidget::OnClickSummon()
{
	RequestMenuChange(EParadiseLobbyMenu::Summon);
}

void UParadiseLobbyMenuPanelWidget::OnClickSquad()
{
	RequestMenuChange(EParadiseLobbyMenu::Squad);
}

void UParadiseLobbyMenuPanelWidget::OnClickEnhance()
{
	RequestMenuChange(EParadiseLobbyMenu::Enhance);
}

void UParadiseLobbyMenuPanelWidget::OnClickCodex()
{
	RequestMenuChange(EParadiseLobbyMenu::Codex);
}

#pragma endregion 로직 구현 (Logic Implementation)
