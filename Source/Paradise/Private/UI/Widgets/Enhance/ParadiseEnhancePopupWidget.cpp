// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Enhance/ParadiseEnhancePopupWidget.h"
#include "UI/Panel/Enhance/ParadiseEnhanceDetailWidget.h"
#include "UI/Widgets/Squad/Inventory/ParadiseSquadInventoryWidget.h"
#include "Components/Button.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/InventorySystem.h"

#pragma region 생명주기
void UParadiseEnhancePopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (CachedGI.IsValid())
	{
		CachedInventorySys = CachedGI->GetSubsystem<UInventorySystem>();
	}

	// 탭 및 닫기 버튼 바인딩
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.AddDynamic(this, &UParadiseEnhancePopupWidget::OnClickCharTab);
	if (Btn_Tab_Weapon) Btn_Tab_Weapon->OnClicked.AddDynamic(this, &UParadiseEnhancePopupWidget::OnClickWpnTab);
	if (Btn_Tab_Armor) Btn_Tab_Armor->OnClicked.AddDynamic(this, &UParadiseEnhancePopupWidget::OnClickArmTab);
	//if (Btn_Tab_Unit) Btn_Tab_Unit->OnClicked.AddDynamic(this, &UParadiseEnhancePopupWidget::OnClickUnitTab);
	if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UParadiseEnhancePopupWidget::HandleClose);

	// 자식 위젯 이벤트 바인딩
	if (Panel_Inventory)
	{
		Panel_Inventory->OnItemClicked.AddDynamic(this, &UParadiseEnhancePopupWidget::HandleInventoryItemClicked);
	}
	if (Panel_Detail)
	{
		Panel_Detail->OnEnhanceClicked.AddDynamic(this, &UParadiseEnhancePopupWidget::RequestEnhance);
		Panel_Detail->OnBreakthroughClicked.AddDynamic(this, &UParadiseEnhancePopupWidget::RequestBreakthrough);
	}

	SwitchTab(SquadTabs::Weapon); // 기본 탭을 무기로 시작
}

void UParadiseEnhancePopupWidget::NativeDestruct()
{
	// 델리게이트 해제
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.RemoveAll(this);
	if (Btn_Tab_Weapon) Btn_Tab_Weapon->OnClicked.RemoveAll(this);
	if (Btn_Tab_Armor) Btn_Tab_Armor->OnClicked.RemoveAll(this);
	//if (Btn_Tab_Unit) Btn_Tab_Unit->OnClicked.RemoveAll(this);
	if (Btn_Close) Btn_Close->OnClicked.RemoveAll(this);

	if (Panel_Inventory) Panel_Inventory->OnItemClicked.RemoveAll(this);
	if (Panel_Detail)
	{
		Panel_Detail->OnEnhanceClicked.RemoveAll(this);
		Panel_Detail->OnBreakthroughClicked.RemoveAll(this);
	}

	// 인벤토리 델리게이트 해제
	if (CachedInventorySys.IsValid())
	{
		CachedInventorySys->OnInventoryUpdated.RemoveAll(this);
	}

	CachedGI = nullptr;
	CachedInventorySys = nullptr;

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 중재 로직
void UParadiseEnhancePopupWidget::SwitchTab(int32 NewTab)
{
	CurrentTabIndex = NewTab;
	SelectedItem = FSquadItemUIData(); // 탭 이동 시 선택 초기화

	if (Panel_Detail) Panel_Detail->ClearDetail();

	RefreshInventory();
}
void UParadiseEnhancePopupWidget::RefreshInventory()
{
	UInventorySystem* InvSys = CachedInventorySys.Get();
	if (!InvSys || !Panel_Inventory) return;

	TArray<FSquadItemUIData> ListData;

	// 임시 테스트용 더미 데이터를 주입하거나, 실제 InventorySystem에서 가져오는 로직
	// (SquadMainWidget에 쓰셨던 switch(CurrentTabIndex) 로직을 여기에 복사해서 넣으시면 됩니다!)

	// 뷰에 전달
	Panel_Inventory->UpdateList(CurrentTabIndex, ListData);
}
void UParadiseEnhancePopupWidget::RefreshAfterEnhancement()
{
	RefreshInventory(); // 리스트 갱신
	if (SelectedItem.InstanceUID.IsValid())
	{
		HandleInventoryItemClicked(SelectedItem); // 디테일 패널 갱신
	}
}
void UParadiseEnhancePopupWidget::HandleInventoryItemClicked(FSquadItemUIData ItemData)
{
	SelectedItem = ItemData;

	if (!Panel_Detail) return;

	/**
	 * @todo 나중에 InventorySystem에서 진짜 스탯/비용 계산 함수를 제공하면 아래 코드를 교체합니다.
	 * 현재는 아키텍처 흐름 확인용 임시 연산입니다.
	 */
	int32 RequiredCost = ItemData.Level * 150;
	FString CurrentStat = FString::Printf(TEXT("레벨 %d\n공격력 %d"), ItemData.Level, ItemData.Level * 10);
	FString NextStat = FString::Printf(TEXT("레벨 %d\n공격력 %d"), ItemData.Level + 1, (ItemData.Level + 1) * 10);

	// 팝업이 데이터를 조립해서 디테일 패널에 "그려!" 라고만 명령합니다.
	Panel_Detail->RefreshDetail(ItemData, CurrentTabIndex, RequiredCost, CurrentStat, NextStat);
}

void UParadiseEnhancePopupWidget::RequestEnhance()
{
	if (!CachedInventorySys.IsValid()) return;
	UE_LOG(LogTemp, Log, TEXT("시스템에 [무기/장비 강화] 요청: UID %s"), *SelectedItem.InstanceUID.ToString());
	// CachedInventorySys->EnhanceItem(SelectedItem.InstanceUID);
}

void UParadiseEnhancePopupWidget::RequestBreakthrough()
{
	if (!CachedInventorySys.IsValid()) return;
	UE_LOG(LogTemp, Log, TEXT("시스템에 [캐릭터 돌파] 요청: UID %s"), *SelectedItem.InstanceUID.ToString());
	// CachedInventorySys->BreakthroughCharacter(SelectedItem.InstanceUID);
}
void UParadiseEnhancePopupWidget::OnClickCharTab() { SwitchTab(SquadTabs::Character); }
void UParadiseEnhancePopupWidget::OnClickWpnTab() { SwitchTab(SquadTabs::Weapon); }
void UParadiseEnhancePopupWidget::OnClickArmTab() { SwitchTab(SquadTabs::Armor); }
//void UParadiseEnhancePopupWidget::OnClickUnitTab() { SwitchTab(SquadTabs::Unit); }
void UParadiseEnhancePopupWidget::HandleClose() { SetVisibility(ESlateVisibility::Collapsed); }
#pragma endregion 중재 로직