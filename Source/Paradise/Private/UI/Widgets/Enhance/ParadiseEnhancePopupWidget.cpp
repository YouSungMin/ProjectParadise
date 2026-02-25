// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Enhance/ParadiseEnhancePopupWidget.h"
#include "UI/Panel/Enhance/ParadiseEnhanceDetailWidget.h"
#include "UI/Widgets/Squad/Inventory/ParadiseSquadInventoryWidget.h"
#include "Components/Button.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/InventorySystem.h"
#include "Data/Structs/ItemStructs.h"
#include "Data/Structs/UnitStructs.h"

#pragma region 헬퍼 함수
/** @brief EItemRarity를 UI 표시용 테두리 태그로 변환합니다. */
static FGameplayTag ConvertRarityToTag(EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::Legendary: return FGameplayTag::RequestGameplayTag("Unit.Rank.S");
	case EItemRarity::Epic:      return FGameplayTag::RequestGameplayTag("Unit.Rank.A");
	case EItemRarity::Rare:      return FGameplayTag::RequestGameplayTag("Unit.Rank.B");
	default:                     return FGameplayTag::RequestGameplayTag("Unit.Rank.C");
	}
}
#pragma endregion 헬퍼 함수

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

	// 실제 InventorySystem에서 데이터를 가져와서 포장합니다!
	switch (CurrentTabIndex)
	{
	case SquadTabs::Character:
		for (const auto& Data : InvSys->GetOwnedCharacters())
		{
			// 강화 인벤토리 리스트에서는 캐릭터 BodyIcon(true) 사용
			FSquadItemUIData UIData = MakeUIData(Data.CharacterID, Data.Level, SquadTabs::Character, true);
			UIData.InstanceUID = Data.CharacterUID;
			ListData.Add(UIData);
		}
		break;

	case SquadTabs::Weapon:
		for (const auto& Data : InvSys->GetOwnedItems())
		{
			if (CachedGI->GetDataTableRow<FWeaponStats>(CachedGI->WeaponStatsDataTable, Data.ItemID))
			{
				FSquadItemUIData UIData = MakeUIData(Data.ItemID, Data.EnhancementLevel, SquadTabs::Weapon);
				UIData.InstanceUID = Data.ItemUID;
				UIData.Quantity = Data.Quantity;
				ListData.Add(UIData);
			}
		}
		break;

	case SquadTabs::Armor:
		for (const auto& Data : InvSys->GetOwnedItems())
		{
			if (CachedGI->GetDataTableRow<FArmorStats>(CachedGI->ArmorStatsDataTable, Data.ItemID))
			{
				FSquadItemUIData UIData = MakeUIData(Data.ItemID, Data.EnhancementLevel, SquadTabs::Armor);
				UIData.InstanceUID = Data.ItemUID;
				UIData.Quantity = Data.Quantity;
				ListData.Add(UIData);
			}
		}
		break;
	}

	// 현재 선택된 아이템 하이라이트 유지 로직 (필수 UX)
	for (auto& Item : ListData)
	{
		if (Item.InstanceUID.IsValid() && Item.InstanceUID == SelectedItem.InstanceUID)
		{
			Item.bIsSelected = true;
		}
	}

	// 뷰(우측 리스트)에 완성된 꽉 찬 택배 상자들 전달!
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

FSquadItemUIData UParadiseEnhancePopupWidget::MakeUIData(FName ID, int32 InLevel, int32 TabType, bool bUseBodyIcon)
{
	FSquadItemUIData Result;
	Result.ID = ID;
	Result.Level = InLevel;
	Result.Name = FText::FromName(ID);

	if (!CachedGI.IsValid()) return Result;

	if (TabType == SquadTabs::Character)
	{
		if (auto* Asset = CachedGI->GetDataTableRow<FCharacterAssets>(CachedGI->CharacterAssetsDataTable, ID))
		{
			// 강화 인벤토리에서도 전신(Body)을 보여주려면 bUseBodyIcon을 활용
			TSoftObjectPtr<UTexture2D> TargetIcon = bUseBodyIcon ? Asset->BodyIcon : Asset->FaceIcon;
			Result.Icon = TargetIcon.LoadSynchronous();
		}
	}
	else if (TabType == SquadTabs::Weapon)
	{
		if (auto* Stat = CachedGI->GetDataTableRow<FWeaponStats>(CachedGI->WeaponStatsDataTable, ID))
		{
			Result.Name = Stat->DisplayName;
			Result.RankTag = ConvertRarityToTag(Stat->Rarity); // 헬퍼 함수 필요(SquadMain 참조)
		}
		if (auto* Asset = CachedGI->GetDataTableRow<FWeaponAssets>(CachedGI->WeaponAssetsDataTable, ID))
		{
			Result.Icon = Asset->Icon.LoadSynchronous();
		}
	}
	else if (TabType == SquadTabs::Armor)
	{
		if (auto* Stat = CachedGI->GetDataTableRow<FArmorStats>(CachedGI->ArmorStatsDataTable, ID))
		{
			Result.Name = Stat->DisplayName;
			Result.RankTag = ConvertRarityToTag(Stat->Rarity);
		}
		if (auto* Asset = CachedGI->GetDataTableRow<FArmorAssets>(CachedGI->ArmorAssetsDataTable, ID))
		{
			Result.Icon = Asset->Icon.LoadSynchronous();
		}
	}

	return Result;
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
void UParadiseEnhancePopupWidget::HandleClose()
{
	// 1. 내부 상태 초기화 (다음에 열릴 때를 대비해 빈 화면으로 만들어둠)
	if (Panel_Detail)
	{
		Panel_Detail->ClearDetail();
	}

	// 2. HUD에게 뒤로가기 요청만 순수하게 전달 (SRP 준수)
	if (OnBackRequested.IsBound())
	{
		OnBackRequested.Broadcast();
	}
}
#pragma endregion 중재 로직