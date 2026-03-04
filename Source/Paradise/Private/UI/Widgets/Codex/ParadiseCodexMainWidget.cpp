// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Codex/ParadiseCodexMainWidget.h"
#include "UI/Widgets/Squad/Inventory/ParadiseSquadInventoryWidget.h"
#include "Components/Button.h"

#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/InventorySystem.h"

#include "Data/Structs/InventoryStruct.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"

#pragma region 생명주기
void UParadiseCodexMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());

	// 버튼 이벤트 바인딩
	if (Btn_Back) Btn_Back->OnClicked.AddDynamic(this, &UParadiseCodexMainWidget::OnClickBack);
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.AddDynamic(this, &UParadiseCodexMainWidget::OnClickTabChar);
	if (Btn_Tab_Weapon) Btn_Tab_Weapon->OnClicked.AddDynamic(this, &UParadiseCodexMainWidget::OnClickTabWeapon);
	if (Btn_Tab_Armor) Btn_Tab_Armor->OnClicked.AddDynamic(this, &UParadiseCodexMainWidget::OnClickTabArmor);
	if (Btn_Tab_Unit) Btn_Tab_Unit->OnClicked.AddDynamic(this, &UParadiseCodexMainWidget::OnClickTabUnit);
	if (Btn_Tab_Misc) Btn_Tab_Misc->OnClicked.AddDynamic(this, &UParadiseCodexMainWidget::OnClickTabMisc);

	// 초기 탭 설정 (캐릭터)
	SwitchTab(SquadTabs::Character);
}

void UParadiseCodexMainWidget::NativeDestruct()
{
	if (Btn_Back) Btn_Back->OnClicked.RemoveAll(this);
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.RemoveAll(this);
	if (Btn_Tab_Weapon) Btn_Tab_Weapon->OnClicked.RemoveAll(this);
	if (Btn_Tab_Armor) Btn_Tab_Armor->OnClicked.RemoveAll(this);
	if (Btn_Tab_Unit) Btn_Tab_Unit->OnClicked.RemoveAll(this);
	if (Btn_Tab_Misc) Btn_Tab_Misc->OnClicked.RemoveAll(this);

	CachedGI = nullptr;
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 내부 로직
UInventorySystem* UParadiseCodexMainWidget::GetInventorySystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UInventorySystem>() : nullptr;
}

void UParadiseCodexMainWidget::SwitchTab(int32 NewTab)
{
	if (CurrentTabIndex == NewTab) return; // 동일 탭 클릭 방지

	CurrentTabIndex = NewTab;
	RefreshCodexList();
}

void UParadiseCodexMainWidget::RefreshCodexList()
{
	UInventorySystem* InvSys = GetInventorySystem();
	if (!InvSys || !CachedGI.IsValid() || !WBP_CodexList) return;

	TArray<FSquadItemUIData> CodexDataList;

	// 각 탭에 맞춰 데이터 테이블의 "모든 데이터"를 긁어옵니다.
	switch (CurrentTabIndex)
	{
	case SquadTabs::Character:
		if (UDataTable* DT = CachedGI->CharacterStatsDataTable)
		{
			for (const auto& RowMap : DT->GetRowMap())
			{
				FName CharID = RowMap.Key;
				
				FSquadItemUIData UIData;
				UIData.ID = CharID;
				UIData.Name = FText::FromName(CharID); 
				UIData.Level = 0; // 도감에서는 레벨 표시 숨김

				// 캐릭터는 인벤토리에 GetCharacterDataByID 함수가 있으므로 활용
				UIData.bIsOwned = (InvSys->GetCharacterDataByID(CharID) != nullptr);

				if (FCharacterAssets* Asset = CachedGI->GetDataTableRow<FCharacterAssets>(CachedGI->CharacterAssetsDataTable, CharID))
				{
					UIData.Icon = Asset->BodyIcon.LoadSynchronous();
				}
				CodexDataList.Add(UIData);
			}
		}
		break;

	case SquadTabs::Weapon:
		if (UDataTable* DT = CachedGI->WeaponStatsDataTable)
		{
			for (const auto& RowMap : DT->GetRowMap())
			{
				FName ItemID = RowMap.Key;
				FWeaponStats* Stat = (FWeaponStats*)RowMap.Value;

				FSquadItemUIData UIData;
				UIData.ID = ItemID;
				UIData.Name = Stat ? Stat->DisplayName : FText::FromName(ItemID);
				UIData.Rarity = Stat ? Stat->Rarity : EItemRarity::Common; 
				UIData.Level = 0; 

				// 무기 보유 검사 (배열 순회)
				bool bHasWeapon = false;
				for (const auto& InvItem : InvSys->GetOwnedItems())
				{
					if (InvItem.ItemID == ItemID) { bHasWeapon = true; break; }
				}
				UIData.bIsOwned = bHasWeapon;

				if (FWeaponAssets* Asset = CachedGI->GetDataTableRow<FWeaponAssets>(CachedGI->WeaponAssetsDataTable, ItemID))
				{
					UIData.Icon = Asset->Icon.LoadSynchronous(); 
				}
				CodexDataList.Add(UIData);
			}
		}
		break;

	case SquadTabs::Armor:
		if (UDataTable* DT = CachedGI->ArmorStatsDataTable)
		{
			for (const auto& RowMap : DT->GetRowMap())
			{
				FName ItemID = RowMap.Key;
				FArmorStats* Stat = (FArmorStats*)RowMap.Value;

				FSquadItemUIData UIData;
				UIData.ID = ItemID;
				UIData.Name = Stat ? Stat->DisplayName : FText::FromName(ItemID);
				UIData.Rarity = Stat ? Stat->Rarity : EItemRarity::Common; 
				UIData.Level = 0;

				// 방어구 보유 검사 (배열 순회)
				bool bHasArmor = false;
				for (const auto& InvItem : InvSys->GetOwnedItems())
				{
					if (InvItem.ItemID == ItemID) { bHasArmor = true; break; }
				}
				UIData.bIsOwned = bHasArmor;

				if (FArmorAssets* Asset = CachedGI->GetDataTableRow<FArmorAssets>(CachedGI->ArmorAssetsDataTable, ItemID))
				{
					UIData.Icon = Asset->Icon.LoadSynchronous(); 
				}
				CodexDataList.Add(UIData);
			}
		}
		break;

	case SquadTabs::Unit:
		if (UDataTable* DT = CachedGI->FamiliarStatsDataTable)
		{
			for (const auto& RowMap : DT->GetRowMap())
			{
				FName FamiliarID = RowMap.Key;
				
				FSquadItemUIData UIData;
				UIData.ID = FamiliarID;
				UIData.Name = FText::FromName(FamiliarID);
				UIData.Level = 0;

				// 함수 대신 GetOwnedFamiliars() 배열을 순회하여 보유 여부를 검사합니다!
				bool bHasFamiliar = false;
				for (const auto& InvFam : InvSys->GetOwnedFamiliars())
				{
					if (InvFam.FamiliarID == FamiliarID)
					{
						bHasFamiliar = true;
						break;
					}
				}
				UIData.bIsOwned = bHasFamiliar;

				if (FFamiliarAssets* Asset = CachedGI->GetDataTableRow<FFamiliarAssets>(CachedGI->FamiliarAssetsDataTable, FamiliarID))
				{
					UIData.Icon = Asset->FaceIcon.LoadSynchronous(); 
				}
				CodexDataList.Add(UIData);
			}
		}
		break;
	case SquadTabs::Misc:
		// 1. 캐릭터 조각(Awakening Pieces) 도감 로드
		if (UDataTable* DT = CachedGI->CharacterStatsDataTable)
		{
			for (const auto& RowMap : DT->GetRowMap())
			{
				FName CharID = RowMap.Key;

				FSquadItemUIData UIData;
				UIData.ID = CharID;
				// 이름 뒤에 '조각'을 붙여서 명확하게 구분해줍니다.
				UIData.Name = FText::Format(FText::FromString(TEXT("{0} 조각")), FText::FromName(CharID));
				UIData.Level = 0;
				UIData.Quantity = 0; // 도감 화면이므로 수량 텍스트 숨김을 위해 0으로 세팅 (WBP_MiscSlot 로직에 따름)
				UIData.Rarity = EItemRarity::Common; // 조각은 보통 공통 등급 테두리를 쓰거나 별도 처리

				// 인벤토리 캐릭터 데이터가 있고, 그 캐릭터의 조각(AwakeningPieces)이 1개 이상일 때 '보유(Owned)' 처리!
				const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(CharID);
				UIData.bIsOwned = (CharData != nullptr && CharData->AwakeningPieces > 0);

				if (FCharacterAssets* Asset = CachedGI->GetDataTableRow<FCharacterAssets>(CachedGI->CharacterAssetsDataTable, CharID))
				{
					// 방금 추가하신 돌파 재화 아이콘을 들고 옵니다.
					UIData.Icon = Asset->AwakeningPieceIcon.LoadSynchronous();
				}

				// 방어 코드: 에셋 테이블에 아직 조각 아이콘을 안 넣은 캐릭터가 있을 수 있으니, 아이콘이 유효할 때만 리스트에 넣습니다.
				if (UIData.Icon)
				{
					CodexDataList.Add(UIData);
				}
			}
		}
		break;
	}

	// 가공이 끝난 도감 전용 데이터를 멍청한 뷰(Dumb View)인 인벤토리 리스트 위젯에 던져줍니다.
	WBP_CodexList->UpdateList(CurrentTabIndex, CodexDataList);
}
#pragma endregion 내부 로직

#pragma region 이벤트 핸들러
void UParadiseCodexMainWidget::OnClickTabChar() { SwitchTab(SquadTabs::Character); }
void UParadiseCodexMainWidget::OnClickTabWeapon() { SwitchTab(SquadTabs::Weapon); }
void UParadiseCodexMainWidget::OnClickTabArmor() { SwitchTab(SquadTabs::Armor); }
void UParadiseCodexMainWidget::OnClickTabUnit() { SwitchTab(SquadTabs::Unit); }
void UParadiseCodexMainWidget::OnClickTabMisc() { SwitchTab(SquadTabs::Misc); }

void UParadiseCodexMainWidget::OnClickBack()
{
	// 1. 패널 상태 강제 초기화 (다음에 도감을 다시 열 때 무조건 '캐릭터 탭'부터 보이도록 세팅)
	CurrentTabIndex = -1; // 강제 업데이트를 위해 인덱스 초기화
	SwitchTab(SquadTabs::Character);

	// 2. 자동 저장
	if (CachedGI.IsValid())
	{
		CachedGI->SaveGameData();
		UE_LOG(LogTemp, Log, TEXT("💾 [CodexMain] 도감을 닫으며 게임 데이터를 자동 저장합니다."));
	}

	if (OnBackRequested.IsBound())
	{
		OnBackRequested.Broadcast();
	}
}
#pragma endregion 이벤트 핸들러