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

#pragma region 헬퍼 함수
/** @brief EItemRarity를 UI 표시용 테두리 태그로 변환합니다. */
FGameplayTag ConvertRarityToTag(EItemRarity Rarity)
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
		UE_LOG(LogTemp, Log, TEXT("[SquadMain] 인벤토리 델리게이트 바인딩 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SquadMain] InventorySystem을 찾을 수 없습니다!"));
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
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UInventorySystem>();
	}
	return nullptr;
}

USquadSubsystem* UParadiseSquadMainWidget::GetSquadSubsystem() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<USquadSubsystem>();
	}
	return nullptr;
}
#pragma endregion 데이터 소스 Getter

#pragma region SquadSubsystem 연동
void UParadiseSquadMainWidget::BindSquadSubsystemDelegates()
{
	USquadSubsystem* SquadSys = GetSquadSubsystem();
	if (!SquadSys)
	{
		UE_LOG(LogTemp, Error, TEXT("[SquadMain] SquadSubsystem을 찾을 수 없어 델리게이트 바인딩을 생략합니다."));
		return;
	}

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
	USquadSubsystem* SquadSys = GetSquadSubsystem();
	if (!SquadSys) return;

	SquadSys->OnPlayerSlotChanged.RemoveAll(this);
	SquadSys->OnFamiliarSlotChanged.RemoveAll(this);

	UE_LOG(LogTemp, Log, TEXT("[SquadMain] SquadSubsystem 델리게이트 해제 완료"));
}

void UParadiseSquadMainWidget::InitFormationFromSubsystem()
{
	USquadSubsystem* SquadSys = GetSquadSubsystem();
	if (!SquadSys || !WBP_FormationPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SquadMain] InitFormationFromSubsystem 실패: SquadSubsystem 또는 FormationPanel 없음"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[SquadMain] 편성 서브시스템 → FormationWidget 초기화 시작"));

	// --- 플레이어 캐릭터 슬롯 (0~2) 동기화 ---
	const TArray<FName>& PlayerIDs = SquadSys->GetPlayerSquad();
	for (int32 i = 0; i < PlayerIDs.Num(); ++i)
	{
		FName PlayerID = PlayerIDs[i];
		// NAME_None이면 빈 슬롯으로 처리 (ID가 없는 FSquadItemUIData 전달)
		FSquadItemUIData UIData = PlayerID.IsNone()
			? FSquadItemUIData()
			: MakeUIData(PlayerID, 1, SquadTabs::Character, true);

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

	UE_LOG(LogTemp, Log, TEXT("[SquadMain] FormationWidget 초기화 완료 (캐릭터 %d슬롯, 유닛 %d슬롯)"),
		PlayerIDs.Num(), FamiliarIDs.Num());
}

void UParadiseSquadMainWidget::OnPlayerSlotUpdated(int32 SlotIndex, FName NewPlayerID)
{
	if (!WBP_FormationPanel) return;

	// SquadSubsystem 플레이어 슬롯(0~2)은 FormationWidget과 인덱스가 동일합니다.
	FSquadItemUIData UIData = NewPlayerID.IsNone()
		? FSquadItemUIData()
		: MakeUIData(NewPlayerID, 1, SquadTabs::Character, true);

	WBP_FormationPanel->UpdateSlot(SlotIndex, UIData);

	UE_LOG(LogTemp, Log, TEXT("[SquadMain] 플레이어 슬롯[%d] 업데이트: %s"),
		SlotIndex, *NewPlayerID.ToString());
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

	UE_LOG(LogTemp, Log, TEXT("[SquadMain] 퍼밀리어 슬롯[%d](Formation[%d]) 업데이트: %s"),
		SlotIndex, FormationIndex, *NewFamiliarID.ToString());
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

	// 선택된 데이터가 있는지 판별
	bool bHasFormationSelection = (SelectedFormationSlotIndex != -1);
	bool bHasInventorySelection = (!PendingSelection.ID.IsNone());

	// 편성 슬롯을 눌렀거나, 인벤토리 아이템을 눌렀을 때만 상세창을 켭니다.
	if (bHasFormationSelection || bHasInventorySelection)
	{
		WBP_DetailPanel->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		// 아무것도 선택하지 않은 상태면 상세창 위젯 전체를 아예 숨겨버립니다.
		WBP_DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
		return; // 패널이 꺼졌으므로 내부 버튼 상태 업데이트는 불필요
	}

	// 켜져 있다면 내부 버튼 가시성을 상태에 맞게 갱신
	bool bIsUnitSlot = (SelectedFormationSlotIndex >= 3 && SelectedFormationSlotIndex <= 7);
	WBP_DetailPanel->UpdateButtonState(CurrentState, bIsUnitSlot, bHasInventorySelection);
}
#pragma endregion 로직 - 탭 및 상태 제어

#pragma region 로직 - 데이터 처리
void UParadiseSquadMainWidget::RefreshInventoryUI()
{
	UInventorySystem* InvSys = GetInventorySystem();
	if (!InvSys || !CachedGI.IsValid() || !WBP_InventoryPanel) return;

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
			ListData.Add(UIData);
		}
		break;
	}

	/** @section 데이터 후처리 (장착 여부 및 선택 상태 표시) */
	for (auto& Item : ListData)
	{
		// TODO: CurrentEquippedIDs에 포함되어 있으면 테두리 표시
		// if (CurrentEquippedIDs.Contains(Item.ID)) Item.bIsEquipped = true;

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
	Result.ID = ID;
	Result.Level = InLevel;
	Result.Name = FText::FromName(ID); // 기본값 (테이블 조회 실패 대비)

	if (!CachedGI.IsValid()) return Result;

	// GameInstance의 템플릿 함수(GetDataTableRow)를 사용하여 데이터 테이블 안전하게 조회
	if (TabType == SquadTabs::Character)
	{
		if (auto* Stat = CachedGI->GetDataTableRow<FCharacterStats>(CachedGI->CharacterStatsDataTable, ID))
		{
			// ★ [수정] 캐릭터에 Rarity 변수가 있다면 ConvertRarityToTag를 호출. 없다면 임시 태그 적용.
			Result.RankTag = FGameplayTag::RequestGameplayTag(TEXT("Unit.Rank.A"));
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
			// ★ [수정] 부모 구조체(FItemBaseStats)의 Rarity를 태그로 변환
			Result.RankTag = ConvertRarityToTag(Stat->Rarity);
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
			Result.RankTag = ConvertRarityToTag(Stat->Rarity);
		}
		if (auto* Asset = CachedGI->GetDataTableRow<FArmorAssets>(CachedGI->ArmorAssetsDataTable, ID))
		{
			Result.Icon = Asset->Icon.LoadSynchronous();
		}
	}
	else if (TabType == SquadTabs::Unit)
	{
		if (auto* Stat = CachedGI->GetDataTableRow<FFamiliarStats>(CachedGI->FamiliarStatsDataTable, ID))
		{
			// 퍼밀리어는 구조체에 RankTypeTag가 있다면 주석 해제 (기존 코드 존중)
			// Result.RankTag = Stat->RankTypeTag;
			Result.RankTag = FGameplayTag::EmptyTag;
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

	// 임시 데이터로 상세창 갱신 테스트
	FSquadItemUIData DummyData;
	DummyData.Name = FText::FromString(bIsUnitSlot ? TEXT("선택된 유닛 슬롯") : TEXT("선택된 캐릭터 슬롯"));
	DummyData.Level = 1;

	// 상세 패널 갱신 (편성창 컨텍스트 = true)
	WBP_DetailPanel->ShowInfo(DummyData, true, bIsUnitSlot);

	// 버튼 상태 업데이트
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

	// TODO: InventoryComponent에 실제 교체 요청
	// if (CurrentState == CharacterSwap) Inventory->EquipCharacter(...)
	// else if (CurrentState == EquipMode) Inventory->EquipItem(...)

	// 교체 완료 후 일반 모드로 복귀
	HandleCancelEquipMode();
}

void UParadiseSquadMainWidget::HandleInventoryItemClicked(FSquadItemUIData ItemData)
{
	// 교체 모드일 때
	if (CurrentState != ESquadUIState::Normal)
	{
		// 1. 선택한 아이템 임시 저장 (Pending)
		PendingSelection = ItemData;

		// 2. 인벤토리 갱신 (선택된 아이템 하이라이트)
		RefreshInventoryUI();

		// 3. 상세 패널 갱신 (확인 버튼 활성화 및 정보 표시)
		if (WBP_DetailPanel)
		{
			// 인벤토리 컨텍스트(false)지만 교체 모드에서는 버튼 상태 업데이트 필요
			WBP_DetailPanel->ShowInfo(ItemData, false, (CurrentTabIndex == SquadTabs::Unit));
			UpdateDetailPanelState(); // 확인 버튼 켜기
		}
	}
	else
	{
		// [일반 모드]
		// 단순 정보 표시 (버튼은 상세 패널 내부 로직에 의해 숨겨짐)
		WBP_DetailPanel->ShowInfo(ItemData, false, (CurrentTabIndex == SquadTabs::Unit));
	}
}

void UParadiseSquadMainWidget::HandleBackClicked()
{
	if (WBP_DetailPanel)
	{
		WBP_DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	// LobbyHUD가 이 이벤트를 받아서 WidgetSwitcher를 메인 메뉴로 전환합니다.
	if (OnBackRequested.IsBound())
	{
		OnBackRequested.Broadcast();
	}
}
#pragma endregion 로직 - 이벤트 핸들러
