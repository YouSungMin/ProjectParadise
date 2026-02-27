// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Enhance/ParadiseEnhancePopupWidget.h"
#include "UI/Panel/Enhance/ParadiseEnhanceDetailWidget.h"
#include "UI/Widgets/Squad/Inventory/ParadiseSquadInventoryWidget.h"
#include "Components/Button.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/System/GrowthSubsystem.h"
#include "Data/Structs/ItemStructs.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/GrowthStruct.h"

//#pragma region 헬퍼 함수
///** @brief EItemRarity를 UI 표시용 테두리 태그로 변환합니다. */
//static FGameplayTag ConvertRarityToTag(EItemRarity Rarity)
//{
//	switch (Rarity)
//	{
//	case EItemRarity::Legendary: return FGameplayTag::RequestGameplayTag("Unit.Rank.S");
//	case EItemRarity::Epic:      return FGameplayTag::RequestGameplayTag("Unit.Rank.A");
//	case EItemRarity::Rare:      return FGameplayTag::RequestGameplayTag("Unit.Rank.B");
//	default:                     return FGameplayTag::RequestGameplayTag("Unit.Rank.C");
//	}
//}
//#pragma endregion 헬퍼 함수

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
			Result.Rarity = Stat->Rarity;
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
			Result.Rarity = Stat->Rarity;
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
	if (!Panel_Detail || !CachedGI.IsValid() || !CachedInventorySys.IsValid()) return;

	int32 RequiredCost = 0;
	FString CurrentStatStr = TEXT("");
	FString NextStatStr = TEXT("");

	// 1. 장비 (무기/방어구) 연산
	if (CurrentTabIndex == SquadTabs::Weapon || CurrentTabIndex == SquadTabs::Armor)
	{
		if (const FOwnedItemData* OwnedItem = CachedInventorySys->GetItemByGUID(ItemData.InstanceUID))
		{
			FName TargetCostID = NAME_None;
			float BaseStatValue = 0.0f;
			FString StatName = (CurrentTabIndex == SquadTabs::Weapon) ? TEXT("공격력") : TEXT("방어력");

			if (CurrentTabIndex == SquadTabs::Weapon)
			{
				if (auto* WpnStats = CachedGI->GetDataTableRow<FWeaponStats>(CachedGI->WeaponStatsDataTable, OwnedItem->ItemID))
				{
					TargetCostID = WpnStats->LevelUpCostId;
					BaseStatValue = WpnStats->AttackPower;
				}
			}
			else
			{
				if (auto* ArmorStats = CachedGI->GetDataTableRow<FArmorStats>(CachedGI->ArmorStatsDataTable, OwnedItem->ItemID))
				{
					TargetCostID = ArmorStats->LevelUpCostId;
					BaseStatValue = ArmorStats->DefensePower;
				}
			}

			if (auto* EnhData = CachedGI->GetDataTableRow<FEquipmentEnhanceData>(CachedGI->EquipmentEnhanceDataTable, TargetCostID))
			{
				int32 CurLv = OwnedItem->EnhancementLevel;

				float CurStat = BaseStatValue * (1.0f + (CurLv * EnhData->StatBonusPerLevel));
				CurrentStatStr = FString::Printf(TEXT("강화 +%d\n%s: %d"), CurLv, *StatName, FMath::RoundToInt(CurStat));

				if (CurLv >= EnhData->MaxEnhanceLevel)
				{
					NextStatStr = TEXT("최대 강화 도달");
					RequiredCost = 0; // 최대 레벨일 경우 Cost를 0으로 설정
				}
				else
				{
					float NxtStat = BaseStatValue * (1.0f + ((CurLv + 1) * EnhData->StatBonusPerLevel));
					NextStatStr = FString::Printf(TEXT("강화 +%d\n%s: %d"), CurLv + 1, *StatName, FMath::RoundToInt(NxtStat));
					RequiredCost = EnhData->BaseGoldCost + (EnhData->GoldCostPerLevel * CurLv);
				}
			}
		}
	}
	// 2. 캐릭터 (돌파) 연산
	else if (CurrentTabIndex == SquadTabs::Character)
	{
		if (const FOwnedCharacterData* OwnedChar = CachedInventorySys->GetCharacterDataByID(ItemData.ID))
		{
			int32 CurAwaken = OwnedChar->AwakeningLevel;
			CurrentStatStr = FString::Printf(TEXT("%d단계 돌파"), CurAwaken);

			FName NextRow = FName(*FString::FromInt(CurAwaken + 1));
			if (auto* NextAwkData = CachedGI->GetDataTableRow<FCharacterAwakenData>(CachedGI->CharacterAwakenDataTable, NextRow))
			{
				NextStatStr = FString::Printf(TEXT("%d단계 돌파\n(최대 레벨 %d 확장)"), CurAwaken + 1, NextAwkData->MaxLevelCap);
				RequiredCost = NextAwkData->RequiredAwakeningPieces;
			}
			else
			{
				NextStatStr = TEXT("최대 돌파 도달");
				RequiredCost = 0; // 최대 레벨일 경우 Cost를 0으로 설정
			}
		}
	}

	// 3. UI 갱신 명령 하달 (Cost 0 전달 시 버튼이 알아서 비활성화됨)
	Panel_Detail->RefreshDetail(ItemData, CurrentTabIndex, RequiredCost, CurrentStatStr, NextStatStr);
}

void UParadiseEnhancePopupWidget::RequestEnhance()
{
	if (!CachedGI.IsValid() || !SelectedItem.InstanceUID.IsValid()) return;

	UGrowthSubsystem* GrowthSys = CachedGI->GetSubsystem<UGrowthSubsystem>();
	if (GrowthSys)
	{
		bool bSuccess = GrowthSys->EnhanceEquipment(SelectedItem.InstanceUID);

		if (Panel_Detail) Panel_Detail->PlayEnhancementFX(bSuccess);
		if (bSuccess) RefreshAfterEnhancement();
	}
}

void UParadiseEnhancePopupWidget::RequestBreakthrough()
{
	if (!CachedGI.IsValid() || SelectedItem.ID.IsNone()) return;

	UGrowthSubsystem* GrowthSys = CachedGI->GetSubsystem<UGrowthSubsystem>();
	if (GrowthSys)
	{
		// 돌파는 InstanceUID가 아닌 원본 FName(ID)로 요청합니다.
		bool bSuccess = GrowthSys->AwakenCharacter(SelectedItem.ID);

		if (Panel_Detail) Panel_Detail->PlayEnhancementFX(bSuccess);
		if (bSuccess) RefreshAfterEnhancement();
	}
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