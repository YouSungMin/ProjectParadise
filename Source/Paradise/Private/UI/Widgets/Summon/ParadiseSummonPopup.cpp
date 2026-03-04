// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Summon/ParadiseSummonPopup.h"
#include "UI/Panel/Summon/ParadiseSummonPanel.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

#pragma region 생명주기
void UParadiseSummonPopup::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 탭 버튼 바인딩
	if (Btn_Tab_Character)
	{
		Btn_Tab_Character->OnClicked.AddDynamic(this, &UParadiseSummonPopup::OnCharacterTabClicked);
	}
	if (Btn_Tab_Equipment)
	{
		Btn_Tab_Equipment->OnClicked.AddDynamic(this, &UParadiseSummonPopup::OnEquipmentTabClicked);
	}

	// 2. 뒤로가기 버튼 바인딩
	if (Btn_Back)
	{
		Btn_Back->OnClicked.AddDynamic(this, &UParadiseSummonPopup::OnBackButtonClicked);
	}
	// 3. 재화 정보 갱신 (GameInstance 등에서 내 정보 가져오기)
	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (CachedGI.IsValid())
	{
		// TODO: GI나 PlayerData에 실제 저장된 변수명으로 교체하세요. (예: GI->GetMyEther())
		// 일단 0이나 임시값으로 테스트
		int32 CurrentEther = 0;
		// CurrentEther = GI->GetPlayerEther(); 

		UpdateAetherUI(CurrentEther);
	}

	// 4. 초기 상태 설정 (캐릭터 탭 기본)
	SwitchTab(INDEX_CHARACTER);
}

void UParadiseSummonPopup::NativeDestruct()
{
	// 델리게이트 안전 해제
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.RemoveAll(this);
	if (Btn_Tab_Equipment) Btn_Tab_Equipment->OnClicked.RemoveAll(this);
	if (Btn_Back) Btn_Back->OnClicked.RemoveAll(this);

	CachedGI = nullptr;
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 내부 로직
void UParadiseSummonPopup::UpdateAetherUI(int32 InEther)
{
	if (Text_AetherAmount)
	{
		// 3자리마다 콤마(,) 찍기 (ex: 1,000,000)
		FNumberFormattingOptions NumberFormat;
		NumberFormat.UseGrouping = true;

		Text_AetherAmount->SetText(FText::AsNumber(InEther, &NumberFormat));
	}
}
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
	if (CachedGI.IsValid())
	{
		CachedGI->SaveGameData();
	}
	// 내 컨트롤러 찾아서 로비(None)로 돌아가달라고 요청
	if (ALobbyPlayerController* PC = GetOwningPlayer<ALobbyPlayerController>())
	{
		// "None"으로 이동하면 -> 카메라는 Main으로, UI는 로비 메뉴로 복구됨
		PC->MoveCameraToMenu(EParadiseLobbyMenu::None);
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
#pragma endregion 내부 로직