// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/ParadiseSquadMainWidget.h"
#include "UI/Widgets/Squad/Inventory/ParadiseSquadInventoryWidget.h"
#include "UI/Widgets/Squad/ParadiseSquadFormationWidget.h"
#include "UI/Widgets/Squad/ParadiseSquadDetailWidget.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/System/SquadSubsystem.h"
#include "Components/Button.h"
#include "Engine/DataTable.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"
#include "Data/Structs/InventoryStruct.h"


#pragma region 생명주기
void UParadiseSquadMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. GameInstance 캐싱 (데이터 테이블 접근)
	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!CachedGI.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SquadMain] GameInstance is invalid! Data loading will fail."));
	}

	// 2. 인벤토리 변경 시 자동 갱신 델리게이트 바인딩
	if (UInventorySystem* InvSys = GetInventorySystem())
	{
		InvSys->OnInventoryUpdated.AddDynamic(this, &UParadiseSquadMainWidget::RefreshInventoryUI);
	}
	// 편성이 확정되는 순간 FormationWidget을 즉시 갱신할 수 있도록 구독합니다.
	BindSquadSubsystemDelegates();

	// 3. 탭 버튼 바인딩
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnClickCharTab);
	if (Btn_Tab_Weapon)    Btn_Tab_Weapon->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnClickWpnTab);
	if (Btn_Tab_Armor)     Btn_Tab_Armor->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnClickArmTab);
	if (Btn_Tab_Unit)      Btn_Tab_Unit->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnClickUnitTab);
	if (Btn_Back)          Btn_Back->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::HandleBackClicked);

	// 4. 자식 위젯 이벤트 구독
	if (WBP_InventoryPanel)
	{
		WBP_InventoryPanel->OnItemClicked.AddDynamic(this, &UParadiseSquadMainWidget::HandleInventoryItemClicked);
	}

	if (WBP_FormationPanel)
	{
		WBP_FormationPanel->OnSlotSelected.AddDynamic(this, &UParadiseSquadMainWidget::HandleFormationSlotSelected);
	}

	if (WBP_DetailPanel)
	{
		WBP_DetailPanel->OnSwapEquipmentClicked.AddDynamic(this, &UParadiseSquadMainWidget::HandleSwapEquipmentMode);
		WBP_DetailPanel->OnCancelClicked.AddDynamic(this, &UParadiseSquadMainWidget::HandleCancelEquipMode);
		WBP_DetailPanel->OnSwapCharacterClicked.AddDynamic(this, &UParadiseSquadMainWidget::HandleSwapCharacterMode);
		WBP_DetailPanel->OnConfirmClicked.AddDynamic(this, &UParadiseSquadMainWidget::HandleConfirmAction);

		WBP_DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 로비에서 이미 편성해 둔 캐릭터/유닛 아이콘이 진입 즉시 보이도록 합니다.
	InitFormationFromSubsystem();

	// 5. 초기 상태 설정 
	ResetPanelState();
}

void UParadiseSquadMainWidget::NativeDestruct()
{
	// 델리게이트 안전 해제
	UnbindSquadSubsystemDelegates();

	if (UInventorySystem* InvSys = GetInventorySystem())
	{
		InvSys->OnInventoryUpdated.RemoveAll(this);
	}

	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.RemoveAll(this);
	if (Btn_Tab_Weapon) Btn_Tab_Weapon->OnClicked.RemoveAll(this);
	if (Btn_Tab_Armor) Btn_Tab_Armor->OnClicked.RemoveAll(this);
	if (Btn_Tab_Unit) Btn_Tab_Unit->OnClicked.RemoveAll(this);
	if (Btn_Back) Btn_Back->OnClicked.RemoveAll(this);

	// 자식 위젯 델리게이트는 위젯 소멸 시 자동 해제되지만, 명시적 해제가 안전함
	if (WBP_InventoryPanel) WBP_InventoryPanel->OnItemClicked.RemoveAll(this);
	if (WBP_FormationPanel) WBP_FormationPanel->OnSlotSelected.RemoveAll(this);
	if (WBP_DetailPanel)
	{
		WBP_DetailPanel->OnSwapCharacterClicked.RemoveAll(this);
		WBP_DetailPanel->OnConfirmClicked.RemoveAll(this);
		WBP_DetailPanel->OnSwapCharacterClicked.RemoveAll(this);
		WBP_DetailPanel->OnConfirmClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 데이터 소스 Getter
UInventorySystem* UParadiseSquadMainWidget::GetInventorySystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UInventorySystem>() : nullptr;
}

USquadSubsystem* UParadiseSquadMainWidget::GetSquadSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<USquadSubsystem>() : nullptr;
}
#pragma endregion 데이터 소스 Getter

#pragma region SquadSubsystem 연동
void UParadiseSquadMainWidget::BindSquadSubsystemDelegates()
{
	USquadSubsystem* SquadSys = GetSquadSubsystem();
	if (!SquadSys) return;

	// 플레이어 슬롯(0~2) 변경 이벤트 구독
	if (!SquadSys->OnPlayerSlotChanged.IsAlreadyBound(this, &UParadiseSquadMainWidget::OnPlayerSlotUpdated))
	{
		SquadSys->OnPlayerSlotChanged.AddDynamic(this, &UParadiseSquadMainWidget::OnPlayerSlotUpdated);
	}

	// 퍼밀리어 슬롯(0~4) 변경 이벤트 구독
	if (!SquadSys->OnFamiliarSlotChanged.IsAlreadyBound(this, &UParadiseSquadMainWidget::OnFamiliarSlotUpdated))
	{
		SquadSys->OnFamiliarSlotChanged.AddDynamic(this, &UParadiseSquadMainWidget::OnFamiliarSlotUpdated);
	}

	UE_LOG(LogTemp, Log, TEXT("[SquadMain] SquadSubsystem 델리게이트 바인딩 완료"));
}

void UParadiseSquadMainWidget::UnbindSquadSubsystemDelegates()
{
	if (USquadSubsystem* SquadSys = GetSquadSubsystem())
	{
		SquadSys->OnPlayerSlotChanged.RemoveAll(this);
		SquadSys->OnFamiliarSlotChanged.RemoveAll(this);
	}
}

void UParadiseSquadMainWidget::InitFormationFromSubsystem()
{
	USquadSubsystem* SquadSys = GetSquadSubsystem();
	if (!SquadSys || !WBP_FormationPanel) return;

	// --- 플레이어 캐릭터 슬롯 (0~2) 동기화 ---
	const TArray<FName>& PlayerIDs = SquadSys->GetPlayerSquad();
	for (int32 i = 0; i < PlayerIDs.Num(); ++i)
	{
		FName PlayerID = PlayerIDs[i];
		// 여기 마지막 인자를 false로 (FaceIcon 사용)
		FSquadItemUIData UIData = PlayerID.IsNone()
			? FSquadItemUIData()
			: MakeUIData(PlayerID, 1, SquadTabs::Character, false);

		WBP_FormationPanel->UpdateSlot(i, UIData);
	}

	// --- 퍼밀리어 슬롯 (SquadSubsystem 0~4 → FormationWidget 3~7) 동기화 ---
	const TArray<FName>& FamiliarIDs = SquadSys->GetFamiliarSquad();
	for (int32 i = 0; i < FamiliarIDs.Num(); ++i)
	{
		FName FamiliarID = FamiliarIDs[i];
		// 퍼밀리어 서브시스템 인덱스(0~4)를 FormationWidget 인덱스(3~7)로 변환
		const int32 FormationIndex = i + 3;
		FSquadItemUIData UIData = FamiliarID.IsNone()
			? FSquadItemUIData()
			: MakeUIData(FamiliarID, 1, SquadTabs::Unit);

		WBP_FormationPanel->UpdateSlot(FormationIndex, UIData);
	}
}

void UParadiseSquadMainWidget::OnPlayerSlotUpdated(int32 SlotIndex, FName NewPlayerID)
{
	if (!WBP_FormationPanel) return;

	// SquadSubsystem 플레이어 슬롯(0~2)은 FormationWidget과 인덱스가 동일합니다.
	FSquadItemUIData UIData = NewPlayerID.IsNone()
		? FSquadItemUIData()
		: MakeUIData(NewPlayerID, 1, SquadTabs::Character, false);

	WBP_FormationPanel->UpdateSlot(SlotIndex, UIData);
}

void UParadiseSquadMainWidget::OnFamiliarSlotUpdated(int32 SlotIndex, FName NewFamiliarID)
{
	if (!WBP_FormationPanel) return;

	// 퍼밀리어 서브시스템 인덱스(0~4) → FormationWidget 인덱스(3~7) 변환
	const int32 FormationIndex = SlotIndex + 3;

	FSquadItemUIData UIData = NewFamiliarID.IsNone()
		? FSquadItemUIData()
		: MakeUIData(NewFamiliarID, 1, SquadTabs::Unit);

	WBP_FormationPanel->UpdateSlot(FormationIndex, UIData);
}
#pragma endregion SquadSubsystem 연동

#pragma region 로직 - 탭 및 상태 제어
void UParadiseSquadMainWidget::OnClickCharTab() { SwitchTab(SquadTabs::Character); }
void UParadiseSquadMainWidget::OnClickWpnTab() { SwitchTab(SquadTabs::Weapon); }
void UParadiseSquadMainWidget::OnClickArmTab() { SwitchTab(SquadTabs::Armor); }
void UParadiseSquadMainWidget::OnClickUnitTab() { SwitchTab(SquadTabs::Unit); }

void UParadiseSquadMainWidget::SwitchTab(int32 NewTab)
{
	// 같은 탭 재클릭 시 무시
	if (CurrentTabIndex == NewTab) return;

	// 장비 교체 모드일 때는 캐릭터/유닛 탭으로 이동 불가 (UI 잠금)
	if (CurrentState == ESquadUIState::EquipmentSwap)
	{
		if (NewTab != SquadTabs::Weapon && NewTab != SquadTabs::Armor) return;
	}
	else if (CurrentState == ESquadUIState::CharacterSwap)
	{
		if (NewTab != SquadTabs::Character && NewTab != SquadTabs::Unit) return;
	}

	CurrentTabIndex = NewTab;

	// 다른 인벤토리 탭을 눌렀을 때 포메이션 선택 해제 및 상세창 닫기
	if (CurrentState == ESquadUIState::Normal)
	{
		SelectedFormationSlotIndex = -1;
		PendingSelection = FSquadItemUIData();

		if (WBP_FormationPanel) WBP_FormationPanel->HighlightSlot(-1);
		if (WBP_DetailPanel) WBP_DetailPanel->ClearInfo();
	}

	// 1. 하단 Detail 패널 버튼 갱신 (유닛 탭은 장비 교체 버튼 숨김 등)
	UpdateDetailPanelState();

	// 2. 인벤토리 리스트 데이터 갱신
	RefreshInventoryUI();
}

void UParadiseSquadMainWidget::UpdateUIState()
{
	bool bIsNormal = (CurrentState == ESquadUIState::Normal);
	bool bIsCharSwap = (CurrentState == ESquadUIState::CharacterSwap);
	bool bIsEquipSwap = (CurrentState == ESquadUIState::EquipmentSwap);

	// 교체 모드 시 관련 없는 탭 비활성화
	if (Btn_Tab_Character)
	{
		bool bEnable = bIsNormal || (bIsCharSwap && CurrentTabIndex == SquadTabs::Character);
		Btn_Tab_Character->SetIsEnabled(bEnable);
	}
	if (Btn_Tab_Unit)
	{
		bool bEnable = bIsNormal || (bIsCharSwap && CurrentTabIndex == SquadTabs::Unit);
		Btn_Tab_Unit->SetIsEnabled(bEnable);
	}
	if (Btn_Tab_Weapon)
	{
		bool bEnable = bIsNormal || bIsEquipSwap;
		Btn_Tab_Weapon->SetIsEnabled(bEnable);
	}
	if (Btn_Tab_Armor)
	{
		bool bEnable = bIsNormal || bIsEquipSwap;
		Btn_Tab_Armor->SetIsEnabled(bEnable);
	}

	// Detail 패널 버튼 모드 전환
	UpdateDetailPanelState();
}

void UParadiseSquadMainWidget::UpdateDetailPanelState()
{
	if (!WBP_DetailPanel) return;

	// 사용자가 명확히 슬롯을 선택했는지 판별
	const bool bHasFormationSelection = (SelectedFormationSlotIndex != -1);
	const bool bHasInventorySelection = (!PendingSelection.ID.IsNone());

	// 편성 슬롯을 눌렀거나, 인벤토리 아이템을 눌렀을 때만 상세창을 켭니다. (탭만 누르면 꺼짐)
	if (bHasFormationSelection || bHasInventorySelection)
	{
		WBP_DetailPanel->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		// 아무것도 선택하지 않은 상태면 상세창 위젯 전체를 아예 숨겨버립니다.
		WBP_DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
		return; // 패널이 꺼졌으므로 버튼 업데이트 생략 (최적화)
	}

	// 현재 탭이나 슬롯이 유닛인지 판별 (장비 교체 버튼 노출 여부 결정)
	bool bIsUnitSlot = false;

	if (bHasFormationSelection)
	{
		bIsUnitSlot = (SelectedFormationSlotIndex >= 3);
	}
	else
	{
		bIsUnitSlot = (CurrentTabIndex == SquadTabs::Unit);
	}

	// 내부 위젯에 데이터 기반 상태 갱신 위임 (Data-Driven)
	WBP_DetailPanel->UpdateButtonState(CurrentState, bIsUnitSlot, bHasInventorySelection);
}
#pragma endregion 로직 - 탭 및 상태 제어

#pragma region 로직 - 데이터 처리
void UParadiseSquadMainWidget::RefreshInventoryUI()
{
	UInventorySystem* InvSys = GetInventorySystem();
	USquadSubsystem* SquadSys = GetSquadSubsystem();
	if (!InvSys || !SquadSys || !CachedGI.IsValid() || !WBP_InventoryPanel) return;

	// 최적화: 장착 마크(bIsEquipped) 판별용 Hash Set 캐싱
	TSet<FName> EquippedCharAndUnitIDs;
	EquippedCharAndUnitIDs.Append(SquadSys->GetPlayerSquad());
	EquippedCharAndUnitIDs.Append(SquadSys->GetFamiliarSquad());

	TSet<FGuid> EquippedItemUIDs;
	for (const auto& CharData : InvSys->GetOwnedCharacters())
	{
		for (const auto& EquipPair : CharData.EquipmentMap)
		{
			if (EquipPair.Value.IsValid()) EquippedItemUIDs.Add(EquipPair.Value);
		}
	}

	TArray<FSquadItemUIData> ListData;

	// 현재 탭에 맞는 데이터를 인벤토리에서 가져와 UI 데이터로 가공
	switch (CurrentTabIndex)
	{
	case SquadTabs::Character:
		for (const auto& Data : InvSys->GetOwnedCharacters())
		{
			// GameInstance의 테이블 조회 로직 활용
			FSquadItemUIData UIData = MakeUIData(Data.CharacterID, Data.Level, SquadTabs::Character, true);
			// [수정] 시스템 제어용 고유 식별자(GUID) 주입
			UIData.InstanceUID = Data.CharacterUID;
			UIData.bIsEquipped = EquippedCharAndUnitIDs.Contains(Data.CharacterID);
			ListData.Add(UIData);
		}
		break;

	case SquadTabs::Weapon:
		for (const auto& Data : InvSys->GetOwnedItems())
		{
			// 무기 테이블에 존재하는 ID만 필터링하여 리스트에 추가
			if (CachedGI->GetDataTableRow<FWeaponStats>(CachedGI->WeaponStatsDataTable, Data.ItemID))
			{
				FSquadItemUIData UIData = MakeUIData(Data.ItemID, Data.EnhancementLevel, SquadTabs::Weapon);
				// [수정] 시스템 제어용 고유 식별자(GUID) 및 수량 주입
				UIData.InstanceUID = Data.ItemUID;
				UIData.Quantity = Data.Quantity;
				UIData.bIsEquipped = EquippedItemUIDs.Contains(Data.ItemUID);
				ListData.Add(UIData);
			}
		}
		break;

	case SquadTabs::Armor:
		for (const auto& Data : InvSys->GetOwnedItems())
		{
			// 방어구 테이블에 존재하는 ID만 필터링
			if (CachedGI->GetDataTableRow<FArmorStats>(CachedGI->ArmorStatsDataTable, Data.ItemID))
			{
				FSquadItemUIData UIData = MakeUIData(Data.ItemID, Data.EnhancementLevel, SquadTabs::Armor);
				// [수정] 시스템 제어용 고유 식별자(GUID) 및 수량 주입
				UIData.InstanceUID = Data.ItemUID;
				UIData.Quantity = Data.Quantity;
				UIData.bIsEquipped = EquippedItemUIDs.Contains(Data.ItemUID);
				ListData.Add(UIData);
			}
		}
		break;

	case SquadTabs::Unit:
		for (const auto& Data : InvSys->GetOwnedFamiliars())
		{
			FSquadItemUIData UIData = MakeUIData(Data.FamiliarID, Data.Level, SquadTabs::Unit);
			// [수정] 시스템 제어용 고유 식별자(GUID)주입
			UIData.InstanceUID = Data.FamiliarUID;
			UIData.bIsEquipped = EquippedCharAndUnitIDs.Contains(Data.FamiliarID);
			ListData.Add(UIData);
		}
		break;
	}

	/** @section 데이터 후처리 (장착 여부 및 선택 상태 표시) */
	for (auto& Item : ListData)
	{
		// [수정] 동일한 아이템이 여러 개일 수 있으므로, 단순 ID(RowName)가 아닌 고유 GUID로 비교합니다.
		if (Item.InstanceUID.IsValid() && Item.InstanceUID == PendingSelection.InstanceUID)
		{
			Item.bIsSelected = true;
		}
	}

	// 가공된 데이터를 뷰(Inventory Panel)에 전달
	WBP_InventoryPanel->UpdateList(CurrentTabIndex, ListData);
}

void UParadiseSquadMainWidget::ResetPanelState()
{
	// 1. 모든 교체 상태 및 펜딩 초기화
	CurrentState = ESquadUIState::Normal;
	PendingSelection = FSquadItemUIData();
	SelectedFormationSlotIndex = -1;

	// 2. 포메이션 하이라이트 해제 및 디테일창 닫기
	if (WBP_FormationPanel) WBP_FormationPanel->HighlightSlot(-1);
	if (WBP_DetailPanel)
	{
		WBP_DetailPanel->ClearInfo();
		WBP_DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 3. 탭 버튼 상태 리셋 및 캐릭터 탭으로 전환
	CurrentTabIndex = -1; // 강제 업데이트 유도
	SwitchTab(SquadTabs::Character);
	UpdateUIState();
}

FSquadItemUIData UParadiseSquadMainWidget::MakeUIData(FName ID, int32 InLevel, int32 TabType, bool bUseBodyIcon)
{
	FSquadItemUIData Result;
	// 1. 방어 코드 및 최적화: ID가 비어있으면 즉시 반환 (테이블 검색 X)
	if (ID.IsNone())
	{
		Result.Name = FText::FromString(TEXT("비어있음"));
		Result.Level = 0;
		return Result;
	}

	Result.ID = ID;
	Result.Level = InLevel;
	Result.Name = FText::FromName(ID); // 기본값 (테이블 조회 실패 대비)

	if (!CachedGI.IsValid()) return Result;

	// GameInstance의 템플릿 함수(GetDataTableRow)를 사용하여 데이터 테이블 안전하게 조회
	if (TabType == SquadTabs::Character)
	{
		if (auto* Stat = CachedGI->GetDataTableRow<FCharacterStats>(CachedGI->CharacterStatsDataTable, ID))
		{
			// 캐릭터는 일단 기본값이나 별도 로직 처리
			Result.Rarity = EItemRarity::Common;
		}
		if (auto* Asset = CachedGI->GetDataTableRow<FCharacterAssets>(CachedGI->CharacterAssetsDataTable, ID))
		{
			TSoftObjectPtr<UTexture2D> TargetIcon = bUseBodyIcon ? Asset->BodyIcon : Asset->FaceIcon;
			Result.Icon = TargetIcon.LoadSynchronous();
		}
	}
	else if (TabType == SquadTabs::Weapon)
	{
		if (auto* Stat = CachedGI->GetDataTableRow<FWeaponStats>(CachedGI->WeaponStatsDataTable, ID))
		{
			Result.Name = Stat->DisplayName;
			// 다이렉트 할당
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
			// ★ [수정] 부모 구조체(FItemBaseStats)의 Rarity를 태그로 변환
			Result.Rarity = Stat->Rarity;
		}
		if (auto* Asset = CachedGI->GetDataTableRow<FArmorAssets>(CachedGI->ArmorAssetsDataTable, ID))
		{
			Result.Icon = Asset->Icon.LoadSynchronous();
		}
	}
	else if (TabType == SquadTabs::Unit)
	{
		// 유닛은 레벨 텍스트가 필요 없음
		Result.Level = 0;
		if (auto* Stat = CachedGI->GetDataTableRow<FFamiliarStats>(CachedGI->FamiliarStatsDataTable, ID))
		{
			Result.Rarity = EItemRarity::Common;
		}
		if (auto* Asset = CachedGI->GetDataTableRow<FFamiliarAssets>(CachedGI->FamiliarAssetsDataTable, ID))
		{
			// 유닛은 무조건 FaceIcon을 씁니다
			Result.Icon = Asset->FaceIcon.LoadSynchronous();
		}
	}

	return Result;
}
#pragma endregion 로직 - 데이터 처리

#pragma region 로직 - 이벤트 핸들러
void UParadiseSquadMainWidget::HandleFormationSlotSelected(int32 SlotIndex)
{
	SelectedFormationSlotIndex = SlotIndex;
	bool bIsUnitSlot = (SlotIndex >= 3);

	// 슬롯 선택 시 상태 초기화 (교체 모드였다면 취소됨)
	if (CurrentState != ESquadUIState::Normal)
	{
		HandleCancelEquipMode();
	}

	USquadSubsystem* SquadSys = GetSquadSubsystem();
	FSquadItemUIData RealData;

	if (SquadSys)
	{
		// 1. 슬롯에 장착된 ID 추출 (배열 인덱스 안전장치 적용)
		FName EquippedID = NAME_None;

		if (!bIsUnitSlot)
		{
			const TArray<FName>& PlayerSquad = SquadSys->GetPlayerSquad();
			if (PlayerSquad.IsValidIndex(SlotIndex)) EquippedID = PlayerSquad[SlotIndex];
		}
		else
		{
			const int32 UnitIndex = SlotIndex - 3;
			const TArray<FName>& FamiliarSquad = SquadSys->GetFamiliarSquad();
			if (FamiliarSquad.IsValidIndex(UnitIndex)) EquippedID = FamiliarSquad[UnitIndex];
		}

		// 2. [최적화] ID가 유효할 때만 MakeUIData를 호출하여 테이블을 조회함
		if (!EquippedID.IsNone())
		{
			const int32 TargetTab = bIsUnitSlot ? SquadTabs::Unit : SquadTabs::Character;
			RealData = MakeUIData(EquippedID, 1, TargetTab, !bIsUnitSlot);
		}
		else
		{
			// 빈 슬롯 처리: 테이블 조회를 완벽하게 스킵하고 하드코딩된 기본값만 전달
			RealData.Name = FText::FromString(TEXT("비어있음"));
			RealData.Level = 0;
		}
	}

	// 3. 디테일 패널 갱신
	ESquadDetailContext ContextToUse = bIsUnitSlot ? ESquadDetailContext::FormationUnit : ESquadDetailContext::FormationCharacter;

	if (WBP_DetailPanel)
	{
		WBP_DetailPanel->ShowInfo(RealData, ContextToUse);
	}

	UpdateDetailPanelState();
}

void UParadiseSquadMainWidget::HandleSwapEquipmentMode()
{
	// 1. 상태 변경 (장비 교체 모드)
	CurrentState = ESquadUIState::EquipmentSwap;
	PendingSelection = FSquadItemUIData(); // 선택 초기화

	// 2. 무기 탭으로 강제 이동
	SwitchTab(SquadTabs::Weapon);

	// 3. UI 잠금 처리 및 버튼 변경
	UpdateUIState();
}

void UParadiseSquadMainWidget::HandleCancelEquipMode()
{
	// 1. 상태 복구 (일반 모드)
	CurrentState = ESquadUIState::Normal;
	PendingSelection = FSquadItemUIData(); // 선택 초기화

	// 2. UI 잠금 해제 및 버튼 복구
	UpdateUIState();

	// 3. 인벤토리 하이라이트 제거
	RefreshInventoryUI();
}

void UParadiseSquadMainWidget::HandleSwapCharacterMode()
{
	// 1. 상태 변경
	CurrentState = ESquadUIState::CharacterSwap;
	PendingSelection = FSquadItemUIData(); // 선택 초기화

	// 2. 탭 강제 이동 (슬롯에 따라 캐릭터 or 유닛)
	bool bIsUnitSlot = (SelectedFormationSlotIndex >= 3);
	SwitchTab(bIsUnitSlot ? SquadTabs::Unit : SquadTabs::Character);

	// 3. UI 잠금 처리
	UpdateUIState();
}

void UParadiseSquadMainWidget::HandleConfirmAction()
{
	if (PendingSelection.ID.IsNone()) return;

	UE_LOG(LogTemp, Log, TEXT("[SquadMain] CONFIRM SWAP: Slot[%d] <-> Item[%s]"), SelectedFormationSlotIndex, *PendingSelection.ID.ToString());

	USquadSubsystem* SquadSys = GetSquadSubsystem();
	if (SquadSys)
	{
		if (CurrentState == ESquadUIState::CharacterSwap)
		{
			if (SelectedFormationSlotIndex < 3)
			{
				SquadSys->SetPlayerToSlot(SelectedFormationSlotIndex, PendingSelection.ID);
			}
			else
			{
				SquadSys->SetFamiliarToSlot(SelectedFormationSlotIndex - 3, PendingSelection.ID);
			}
		}
		else if (CurrentState == ESquadUIState::EquipmentSwap)
		{
			if (UInventorySystem* InvSys = GetInventorySystem())
			{
				FName TargetCharacterID = NAME_None;
				const TArray<FName>& PlayerSquad = SquadSys->GetPlayerSquad();

				if (PlayerSquad.IsValidIndex(SelectedFormationSlotIndex))
				{
					TargetCharacterID = PlayerSquad[SelectedFormationSlotIndex];
				}

				if (!TargetCharacterID.IsNone() && PendingSelection.InstanceUID.IsValid())
				{
					if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(TargetCharacterID))
					{
						InvSys->EquipItemToCharacter(CharData->CharacterUID, PendingSelection.InstanceUID);

						UE_LOG(LogTemp, Warning, TEXT("✅ [SquadMain] [%s] 캐릭터에게 장비[%s] 장착 로직 호출 완료!"),
							*TargetCharacterID.ToString(), *PendingSelection.ID.ToString());
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("❌ [SquadMain] 인벤토리에서 캐릭터[%s] 데이터를 찾을 수 없어 장비 장착에 실패했습니다."), *TargetCharacterID.ToString());
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠️ [SquadMain] 타겟 캐릭터가 편성되어 있지 않거나, 선택한 장비의 UID가 유효하지 않습니다."));
				}
			}
		}
	}

	HandleCancelEquipMode();

	// 2. 그 다음 DetailPanel 장비 아이콘 갱신
	// (이제 CurrentState가 Normal이므로 HandleFormationSlotSelected 안에서 HandleCancelEquipMode를 다시 호출하지 않음!)
	if (WBP_DetailPanel && SelectedFormationSlotIndex != -1)
	{
		HandleFormationSlotSelected(SelectedFormationSlotIndex);
	}
}

void UParadiseSquadMainWidget::HandleInventoryItemClicked(FSquadItemUIData ItemData)
{
	// 1. Context 한 번만 깔끔하게 판별
	ESquadDetailContext TargetContext = ESquadDetailContext::InventoryCharacter;
	switch (CurrentTabIndex)
	{
	case SquadTabs::Weapon: TargetContext = ESquadDetailContext::InventoryWeapon; break;
	case SquadTabs::Armor:  TargetContext = ESquadDetailContext::InventoryArmor;  break;
	case SquadTabs::Unit:   TargetContext = ESquadDetailContext::InventoryUnit;   break;
	}

	// 2. 무조건 데이터 저장
	PendingSelection = ItemData;

	// 3. 일반 모드일 경우에만 편성창 하이라이트 해제 추가 로직 실행
	if (CurrentState == ESquadUIState::Normal)
	{
		SelectedFormationSlotIndex = -1;
		if (WBP_FormationPanel) WBP_FormationPanel->HighlightSlot(-1);
	}

	// 4. 공통 렌더링 지시 (중복 제거)
	RefreshInventoryUI();
	if (WBP_DetailPanel)
	{
		WBP_DetailPanel->ShowInfo(ItemData, TargetContext);
	}
	UpdateDetailPanelState();
}

void UParadiseSquadMainWidget::HandleBackClicked()
{
	ResetPanelState();

	if (bAutoSaveOnBack)
	{
		if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
		{
			GI->SaveGameData();
		}
	}
	// LobbyHUD가 이 이벤트를 받아서 WidgetSwitcher를 메인 메뉴로 전환합니다.
	if (OnBackRequested.IsBound())
	{
		OnBackRequested.Broadcast();
	}
}
#pragma endregion 로직 - 이벤트 핸들러
