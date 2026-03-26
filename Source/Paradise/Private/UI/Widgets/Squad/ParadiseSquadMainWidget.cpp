// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/ParadiseSquadMainWidget.h"
#include "UI/Widgets/Squad/Inventory/ParadiseSquadInventoryWidget.h"
#include "UI/Widgets/Squad/ParadiseSquadFormationWidget.h"
#include "UI/Widgets/Common/ParadiseResourceWarningWidget.h"
#include "UI/Widgets/Squad/ParadiseSquadDetailWidget.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/System/SquadSubsystem.h"
#include "Actors/Squad/ParadiseSquadSceneManager.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/AudioComponent.h"
#include "Engine/DataTable.h"
#include "Data/Assets/ParadiseFXAudioData.h"
#include "Data/Assets/FXDataAsset.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"
#include "Data/Structs/InventoryStruct.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"


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

	SceneManager = Cast<AParadiseSquadSceneManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AParadiseSquadSceneManager::StaticClass()));
	if (!SceneManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [SquadMain] 맵에 배치된 AParadiseSquadSceneManager를 찾을 수 없습니다. 3D 모델링이 표시되지 않습니다."));
	}
	// 3. 탭 버튼 바인딩
	if (Btn_Tab_Character) Btn_Tab_Character->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnClickCharTab);
	if (Btn_Tab_Weapon)    Btn_Tab_Weapon->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnClickWpnTab);
	if (Btn_Tab_Armor)     Btn_Tab_Armor->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnClickArmTab);
	if (Btn_Tab_Unit)      Btn_Tab_Unit->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnClickUnitTab);
	if (Btn_Back)          Btn_Back->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::HandleBackClicked);
	// 3D 터치용 투명 버튼 바인딩
	if (Btn_Hitbox_Main) Btn_Hitbox_Main->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnTouch3DCharacter_Main);
	if (Btn_Hitbox_Sub1) Btn_Hitbox_Sub1->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnTouch3DCharacter_Sub1);
	if (Btn_Hitbox_Sub2) Btn_Hitbox_Sub2->OnClicked.AddDynamic(this, &UParadiseSquadMainWidget::OnTouch3DCharacter_Sub2);

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
		WBP_DetailPanel->OnSellClicked.AddDynamic(this, &UParadiseSquadMainWidget::HandleSellAction);

		WBP_DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Anim_TouchBlink)
	{
		PlayAnimation(Anim_TouchBlink, 0.0f, 0);
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
	if (Btn_Hitbox_Main) Btn_Hitbox_Main->OnClicked.RemoveAll(this);
	if (Btn_Hitbox_Sub1) Btn_Hitbox_Sub1->OnClicked.RemoveAll(this);
	if (Btn_Hitbox_Sub2) Btn_Hitbox_Sub2->OnClicked.RemoveAll(this);

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

void UParadiseSquadMainWidget::RefreshDetailPanelForCurrentSlot()
{
	if (SelectedFormationSlotIndex == -1 || !GetSquadSubsystem()) return;

	bool bIsUnitSlot = (SelectedFormationSlotIndex >= 3);
	FName EquippedID = NAME_None;

	if (!bIsUnitSlot) EquippedID = GetSquadSubsystem()->GetPlayerSquad().IsValidIndex(SelectedFormationSlotIndex) ? GetSquadSubsystem()->GetPlayerSquad()[SelectedFormationSlotIndex] : NAME_None;
	else EquippedID = GetSquadSubsystem()->GetFamiliarSquad().IsValidIndex(SelectedFormationSlotIndex - 3) ? GetSquadSubsystem()->GetFamiliarSquad()[SelectedFormationSlotIndex - 3] : NAME_None;

	int32 RealLevel = 0;
	if (!bIsUnitSlot && GetInventorySystem())
	{
		if (const FOwnedCharacterData* CharData = GetInventorySystem()->GetCharacterDataByID(EquippedID)) RealLevel = CharData->Level;
	}

	FSquadItemUIData RealData = MakeUIData(EquippedID, RealLevel, bIsUnitSlot ? SquadTabs::Unit : SquadTabs::Character, !bIsUnitSlot);
	ESquadDetailContext ContextToUse = bIsUnitSlot ? ESquadDetailContext::FormationUnit : ESquadDetailContext::FormationCharacter;

	if (WBP_DetailPanel) WBP_DetailPanel->ShowInfo(RealData, ContextToUse);

	UpdateDetailPanelState();
}

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

		//0303 김성현 - 하드코딩 된 레벨변수 가져오도록 변경
		int32 CurrentLevel = 1;
		if (UInventorySystem* InvSys = GetInventorySystem())
		{
			if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(PlayerID))
			{
				CurrentLevel = CharData->Level;
			}
		}

		// 여기 마지막 인자를 false로 (FaceIcon 사용)
		FSquadItemUIData UIData = PlayerID.IsNone()
			? FSquadItemUIData()
			: MakeUIData(PlayerID, CurrentLevel, SquadTabs::Character, false);

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
	if (SceneManager) SceneManager->RefreshSquadScene();
}

void UParadiseSquadMainWidget::OnPlayerSlotUpdated(int32 SlotIndex, FName NewPlayerID)
{
	if (!WBP_FormationPanel) return;

	// 캐릭터 슬롯이므로 인라인으로 실제 레벨 추출
	int32 RealLevel = 1;
	if (UInventorySystem* InvSys = GetInventorySystem())
	{
		if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(NewPlayerID))
		{
			RealLevel = CharData->Level;
		}
	}

	FSquadItemUIData UIData = NewPlayerID.IsNone()
		? FSquadItemUIData()
		: MakeUIData(NewPlayerID, RealLevel, SquadTabs::Character, false);

	WBP_FormationPanel->UpdateSlot(SlotIndex, UIData);

	// 편성 변경 시 3D 디오라마 실시간 동기화
	if (SceneManager) SceneManager->RefreshSquadScene();
}

void UParadiseSquadMainWidget::OnFamiliarSlotUpdated(int32 SlotIndex, FName NewFamiliarID)
{
	if (!WBP_FormationPanel) return;

	const int32 FormationIndex = SlotIndex + 3;

	// 퍼밀리어 슬롯은 레벨이 필요 없으므로 연산 없이 무조건 0 전달 (최적화)
	FSquadItemUIData UIData = NewFamiliarID.IsNone()
		? FSquadItemUIData()
		: MakeUIData(NewFamiliarID, 0, SquadTabs::Unit);

	WBP_FormationPanel->UpdateSlot(FormationIndex, UIData);
}
#pragma endregion SquadSubsystem 연동

#pragma region 로직 - 탭 및 상태 제어
void UParadiseSquadMainWidget::OnClickCharTab() { SwitchTab(SquadTabs::Character); }
void UParadiseSquadMainWidget::OnClickWpnTab() { SwitchTab(SquadTabs::Weapon); }
void UParadiseSquadMainWidget::OnClickArmTab() { SwitchTab(SquadTabs::Armor); }
void UParadiseSquadMainWidget::OnClickUnitTab() { SwitchTab(SquadTabs::Unit); }
void UParadiseSquadMainWidget::OnTouch3DCharacter_Main() { HandleFormationSlotSelected(0); }
void UParadiseSquadMainWidget::OnTouch3DCharacter_Sub1() { HandleFormationSlotSelected(1); }
void UParadiseSquadMainWidget::OnTouch3DCharacter_Sub2() { HandleFormationSlotSelected(2); }

void UParadiseSquadMainWidget::SwitchTab(int32 NewTab)
{
	// 같은 탭 재클릭 시 무시
	if (CurrentTabIndex == NewTab) return;

	// 탭 변경 공통 효과음 재생
	if (CachedGI.IsValid() && CachedGI->GlobalAudioData && CachedGI->GlobalAudioData->SFX_CommonTabClick)
	{
		UGameplayStatics::PlaySound2D(this, CachedGI->GlobalAudioData->SFX_CommonTabClick);
	}

	CurrentTabIndex = NewTab;
	bool bIsUnitTab = (NewTab == SquadTabs::Unit);

	// 1. 탭에 따라 "마지막으로 보던 슬롯"을 기억에서 꺼내어 복구합니다!
	SelectedFormationSlotIndex = bIsUnitTab ? LastSelectedUnitSlot : LastSelectedCharacterSlot;

	// 2. 탭에 맞게 교체 모드 상태 변경
	if (bIsUnitTab) CurrentState = ESquadUIState::CharacterSwap;
	else if (NewTab == SquadTabs::Weapon || NewTab == SquadTabs::Armor) CurrentState = ESquadUIState::EquipmentSwap;
	else CurrentState = ESquadUIState::CharacterSwap;

	// 3. 탭이 바뀌었으니 인벤토리 선택 대기열은 싹 비워줍니다.
	PendingSelection = FSquadItemUIData();

	// 4. 복구된 슬롯(SelectedFormationSlotIndex)을 기준으로 디테일 창을 즉시 새로고침!
	RefreshDetailPanelForCurrentSlot();

	// 5. 인벤토리 리스트 새로 렌더링
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
			FSquadItemUIData UIData = MakeUIData(Data.CharacterID, Data.Level, SquadTabs::Character, false);
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
	// 외부 호출용 래퍼: 스위처 0번 복귀 및 데이터 초기화
	ReturnToOverviewScreen();

	CurrentTabIndex = -1; // 강제 갱신 유도
	SwitchTab(SquadTabs::Character);
}

void UParadiseSquadMainWidget::OnEnterSquadUI()
{
	// 1. 패널 상태 초기화
	ResetPanelState();

	// 카메라를 3D 스튜디오(SceneManager)로 즉시 컷 전환!
	if (APlayerController* PC = GetOwningPlayer())
	{
		// 돌아갈 로비 카메라 시점 백업
		OriginalViewTarget = PC->GetViewTarget();

		if (SceneManager)
		{
			PC->SetViewTarget(SceneManager);

			// 혹시 모르니 들어올 때마다 마네킹 옷도 새로고침
			SceneManager->RefreshSquadScene();
		}
	}
}
void UParadiseSquadMainWidget::ReturnToOverviewScreen()
{
	if (Switcher_MainScreen)
	{
		Switcher_MainScreen->SetActiveWidgetIndex(0);
	}

	CurrentState = ESquadUIState::Normal;
	SelectedFormationSlotIndex = -1;
	PendingSelection = FSquadItemUIData();

	if (WBP_FormationPanel) WBP_FormationPanel->HighlightSlot(-1);
	if (WBP_DetailPanel)
	{
		WBP_DetailPanel->ClearInfo();
		WBP_DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Anim_TouchBlink)
	{
		PlayAnimation(Anim_TouchBlink, 0.0f, 0);
	}

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
			Result.Name = FText::FromName(ID);
		}
		if (auto* Asset = CachedGI->GetDataTableRow<FCharacterAssets>(CachedGI->CharacterAssetsDataTable, ID))
		{
			Result.Rarity = Asset->Rarity;

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
			// 부모 구조체(FItemBaseStats)의 Rarity를 태그로 변환
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
	bool bIsUnitSlot = (SlotIndex >= 3);

	// 1. 사용자가 누른 슬롯 번호를 탭 복구용 "메모리"에 저장해둡니다!
	if (bIsUnitSlot)
	{
		LastSelectedUnitSlot = SlotIndex;
	}
	else
	{
		LastSelectedCharacterSlot = SlotIndex;
	}

	// 현재 선택된 슬롯 인덱스 갱신
	SelectedFormationSlotIndex = SlotIndex;

	// 2. 슬롯 터치 시 즉시 1번 캔버스(인벤토리/디테일 화면)로 진입
	if (Switcher_MainScreen)
	{
		Switcher_MainScreen->SetActiveWidgetIndex(1);
	}

	int32 TargetTab = bIsUnitSlot ? SquadTabs::Unit : SquadTabs::Character;

	// 3. 탭 변경 로직 분기
	if (CurrentTabIndex != TargetTab)
	{
		// 탭이 달라져야 한다면 SwitchTab을 호출합니다. 
		// (SwitchTab 안에서 RefreshDetailPanelForCurrentSlot이 자동으로 불립니다)
		SwitchTab(TargetTab);
	}
	else
	{
		CurrentState = ESquadUIState::CharacterSwap;

		PendingSelection = FSquadItemUIData();
		RefreshDetailPanelForCurrentSlot();
		RefreshInventoryUI();
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
	ReturnToOverviewScreen();
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

	USquadSubsystem* SquadSys = GetSquadSubsystem();
	if (SquadSys)
	{
		/** @section 실제 교체 및 사운드 연출 */
		if (CurrentState == ESquadUIState::CharacterSwap)
		{
			bool bIsUnitSlot = (SelectedFormationSlotIndex >= 3);

			if (!bIsUnitSlot) // 1. [플레이어 캐릭터] 교체!
			{
				SquadSys->SetPlayerToSlot(SelectedFormationSlotIndex, PendingSelection.ID);

				// 캐릭터 고유 음성(Voice) 재생 로직
				if (CachedGI.IsValid())
				{
					if (FCharacterAssets* CharAsset = CachedGI->GetDataTableRow<FCharacterAssets>(CachedGI->CharacterAssetsDataTable, PendingSelection.ID))
					{
						// 1. CharacterFX 구조체를 한 번 거쳐서 데이터 에셋을 로드합니다.
						if (UFXDataAsset* LoadedFXData = CharAsset->CharacterFX.FXData.LoadSynchronous())
						{
							// 2. 보이스 태그를 받음
							FGameplayTag VoiceTag = CharAsset->CharacterFX.UltimateTag;

							if (VoiceTag.IsValid())
							{
								// 3. 받은 보이스 태그로 사운드 인펙트를 찾음
								if (FFXPayload* FoundFX = LoadedFXData->FindEffect(VoiceTag))
								{
									// 4. 사운드 재생 
									if (USoundBase* LoadedVoice = FoundFX->SoundEffect.LoadSynchronous())
									{
										// 5. 사운드 겹침(Overlap) 방지: 기존 소리가 재생 중이면 끕니다.
										if (CurrentVoiceComponent && CurrentVoiceComponent->IsPlaying())
										{
											CurrentVoiceComponent->Stop();
										}
										// 6. 새로운 캐릭터의 목소리를 스폰하고, 제어권을 컴포넌트 포인터에 저장합니다.
										CurrentVoiceComponent = UGameplayStatics::SpawnSound2D(this, LoadedVoice);
									}
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("⚠️ [SquadMain] %s의 FXDataAsset 안에 %s 태그와 매칭되는 이펙트(Payload)가 없습니다!"), *PendingSelection.ID.ToString(), *VoiceTag.ToString());
								}
							}
							else
							{
								UE_LOG(LogTemp, Error, TEXT("❌ [SquadMain] %s 캐릭터의 데이터 테이블(CharacterFX.UltimateTag)이 비어있습니다! 에디터에서 세팅해주세요."), *PendingSelection.ID.ToString());
							}
						}
					}
				}
			}
			else // 2. [퍼밀리어 유닛] 교체!
			{
				SquadSys->SetFamiliarToSlot(SelectedFormationSlotIndex - 3, PendingSelection.ID);

				if (CachedGI.IsValid() && CachedGI->GlobalAudioData && CachedGI->GlobalAudioData->SFX_FamiliarEquip)
				{
					UGameplayStatics::PlaySound2D(this, CachedGI->GlobalAudioData->SFX_FamiliarEquip);
				}
			}
		}
		else if (CurrentState == ESquadUIState::EquipmentSwap) // 3. [무기/방어구] 교체!
		{
			if (UInventorySystem* InvSys = GetInventorySystem())
			{
				FName TargetCharacterID = NAME_None;
				const TArray<FName>& PlayerSquad = SquadSys->GetPlayerSquad();
				if (PlayerSquad.IsValidIndex(SelectedFormationSlotIndex)) TargetCharacterID = PlayerSquad[SelectedFormationSlotIndex];

				if (!TargetCharacterID.IsNone() && PendingSelection.InstanceUID.IsValid())
				{
					if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(TargetCharacterID))
					{
						InvSys->EquipItemToCharacter(CharData->CharacterUID, PendingSelection.InstanceUID);
					}
				}
			}

			// 무기/장비 장착 공통 효과음 재생
			if (CachedGI.IsValid() && CachedGI->GlobalAudioData && CachedGI->GlobalAudioData->SFX_FamiliarEquip)
			{
				UGameplayStatics::PlaySound2D(this, CachedGI->GlobalAudioData->SFX_FamiliarEquip);
			}
		}
	}



	// 3D 마네킹 실시간 옷 갈아입히기
	if (SceneManager) SceneManager->RefreshSquadScene();

	PendingSelection = FSquadItemUIData();

	//캐릭터 스탯 갱신 완료!
	RefreshDetailPanelForCurrentSlot();

	RefreshInventoryUI();

}

void UParadiseSquadMainWidget::HandleSellAction()
{
	//인벤토리에서 선택해둔 아이템(고유 식별자)이 있는지 확인
	if (!PendingSelection.InstanceUID.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [SquadMain] 판매할 아이템이 선택되지 않았습니다."));
		return;
	}

	//인벤토리 시스템 가져오기
	UInventorySystem* InvSys = GetInventorySystem();
	if (!InvSys) return;

	//판매 로직 호출 (장비/무기는 보통 1개씩이므로 수량을 1로 고정)
	FString ErrorMsg;
	FString SuccessMsg =TEXT("아이템이 성공적으로 판매되었습니다.");
	bool bSellSuccess = InvSys->SellItem(PendingSelection.InstanceUID, 1, ErrorMsg);

	if (bSellSuccess)
	{
		if (Widget_Warning)
		{
			Widget_Warning->ShowWarning(FText::FromString(SuccessMsg), nullptr, true);
		}
		//UE_LOG(LogTemp, Log, TEXT("✅ [SquadMain] 아이템이 성공적으로 판매되었습니다."));

		//[판매 성공 시 후처리]
		//아이템이 사라졌으므로 현재 선택 상태를 초기화
		PendingSelection = FSquadItemUIData();

		// 우측의 디테일 정보 패널을 초기화
		if (WBP_DetailPanel)
		{
			WBP_DetailPanel->ClearInfo();
		}

		//최신상태 갱신
		UpdateDetailPanelState();

	}
	else
	{
		if (Widget_Warning)
		{
			Widget_Warning->ShowWarning(FText::FromString(ErrorMsg),nullptr ,true);
		}
		//UE_LOG(LogTemp, Warning, TEXT("🚫 [SquadMain] 판매 실패: %s"), *ErrorMsg);
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
	// 재생 중인 캐릭터 음성 끄기
	if (CurrentVoiceComponent && CurrentVoiceComponent->IsPlaying())
	{
		CurrentVoiceComponent->Stop();
	}
	// 뒤로가기 공통 효과음 재생
	if (CachedGI.IsValid() && CachedGI->GlobalAudioData && CachedGI->GlobalAudioData->SFX_CommonBack)
	{
		UGameplayStatics::PlaySound2D(this, CachedGI->GlobalAudioData->SFX_CommonBack);
	}
	// 스위처 상태에 따른 네비게이션 분기 처리
	if (Switcher_MainScreen && Switcher_MainScreen->GetActiveWidgetIndex() == 1)
	{
		// 인벤토리 화면(1번)에 있다면 -> 메인 3D 화면(0번)으로 복귀!
		ReturnToOverviewScreen();
	}
	else
	{
		// 메인 3D 화면(0번)에 있다면 -> 로비로 완전 퇴장!
		ResetPanelState();

		if (APlayerController* PC = GetOwningPlayer())
		{
			if (OriginalViewTarget)
			{
				PC->SetViewTarget(OriginalViewTarget);
			}
		}

		if (bAutoSaveOnBack && CachedGI.IsValid()) CachedGI->SaveGameData();
		if (OnBackRequested.IsBound()) OnBackRequested.Broadcast();
	}
}
#pragma endregion 로직 - 이벤트 핸들러
