// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/ParadiseSquadDetailWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/UnitStructs.h"

#pragma region 생명주기
void UParadiseSquadDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_SwapCharacter)   Btn_SwapCharacter->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleSwapChar);
	if (Btn_SwapEquipment)   Btn_SwapEquipment->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleSwapEquip);
	if (Btn_CancelEquipMode) Btn_CancelEquipMode->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleCancel);
	if (Btn_Confirm)         Btn_Confirm->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleConfirm);

	SetVisibility(ESlateVisibility::Collapsed);
}

void UParadiseSquadDetailWidget::NativeDestruct()
{
	if (Btn_SwapCharacter)   Btn_SwapCharacter->OnClicked.RemoveAll(this);
	if (Btn_SwapEquipment)   Btn_SwapEquipment->OnClicked.RemoveAll(this);
	if (Btn_CancelEquipMode) Btn_CancelEquipMode->OnClicked.RemoveAll(this);
	if (Btn_Confirm)         Btn_Confirm->OnClicked.RemoveAll(this);

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 공개 함수
void UParadiseSquadDetailWidget::ShowInfo(const FSquadItemUIData& InData, ESquadDetailContext InContext)
{
	SetVisibility(ESlateVisibility::Visible);

	// 1. 공통 데이터 렌더링 (이름, 메인 아이콘)
	if (Text_Name)
	{
		Text_Name->SetText(InData.Name.IsEmpty() ? FText::FromString(TEXT("-")) : InData.Name);
	}

	if (Img_Icon)
	{
		// 🚨 [최적화 & 폴백 적용] 데이터에 아이콘이 있으면 그걸 쓰고, 없으면 DefaultMainIcon을 씁니다.
		UTexture2D* TargetIcon = InData.Icon ? InData.Icon : DefaultMainIcon;

		if (TargetIcon)
		{
			Img_Icon->SetBrushFromTexture(TargetIcon);
			Img_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			// 폴백 이미지조차 안 넣어놨다면 어쩔 수 없이 숨깁니다.
			Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 2. 레이아웃 초기화 (모든 추가 컨테이너를 우선 숨김)
	if (Container_EquippedItems) Container_EquippedItems->SetVisibility(ESlateVisibility::Collapsed);
	if (Container_Skill)         Container_Skill->SetVisibility(ESlateVisibility::Collapsed);

	// 3. 컨텍스트(Context)에 따른 동적 레이아웃 활성화 및 스탯 문자열 조립 (Data-Driven)
	FString FinalStatString = TEXT("");
	bool bShowActionButtons = false;
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	UInventorySystem* InvSys = GI ? GI->GetSubsystem<UInventorySystem>() : nullptr;

	switch (InContext)
	{
	case ESquadDetailContext::FormationCharacter:
		if (Container_EquippedItems) Container_EquippedItems->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (Container_Skill)         Container_Skill->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		FinalStatString = FString::Printf(TEXT("Lv.%d\n캐릭터 기본 스탯 반영"), FMath::Max(1, InData.Level));
		bShowActionButtons = true;

		// 장착된 무기/방어구 5개의 아이콘을 실시간 바인딩
		if (InvSys)
		{
			if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(InData.ID))
			{
				//0227 김성현 - 슬롯 Enum 수정에따라 코드 수정
				UpdateEquipmentIcon(EEquipmentSlot::Weapon, Img_EquipWeapon, CharData->EquipmentMap);
				UpdateEquipmentIcon(EEquipmentSlot::Hat, Img_EquipHelmet, CharData->EquipmentMap);
				UpdateEquipmentIcon(EEquipmentSlot::Armor, Img_EquipChest, CharData->EquipmentMap);
				UpdateEquipmentIcon(EEquipmentSlot::Necklace, Img_EquipAcc1, CharData->EquipmentMap);
				UpdateEquipmentIcon(EEquipmentSlot::Ring, Img_EquipAcc2, CharData->EquipmentMap);
			}
		}

		// 캐릭터 궁극기 아이콘 바인딩 (추후 ActionStats 테이블 연동)
		if (Img_SkillIcon && GI)
		{
			// if (FCharacterStats* CharStat = GI->GetDataTableRow<FCharacterStats>(GI->CharacterStatsDataTable, InData.ID)) { ... }
		}
		break;

	case ESquadDetailContext::InventoryCharacter:
		if (Container_Skill) Container_Skill->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FinalStatString = FString::Printf(TEXT("Lv.%d\n체력: -- / 공격력: --"), FMath::Max(1, InData.Level));
		break;

	case ESquadDetailContext::InventoryWeapon:
		if (Container_Skill) Container_Skill->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FinalStatString = FString::Printf(TEXT("강화 +%d\n공격력: -- / 치명타: --"), InData.Level);
		break;

	case ESquadDetailContext::InventoryArmor:
		FinalStatString = FString::Printf(TEXT("강화 +%d\n방어력: --"), InData.Level);
		break;

	case ESquadDetailContext::FormationUnit:
		bShowActionButtons = true;
	case ESquadDetailContext::InventoryUnit:
		FinalStatString = TEXT("체력: -- / 공격력: --\n(유닛 특수 능력치)");
		break;
	}

	// 4. 최종 스탯 텍스트 적용
	if (Text_Desc) Text_Desc->SetText(FText::FromString(FinalStatString));

	// 5. 하단 버튼 노출 제어
	if (HBox_ButtonRoot)
	{
		HBox_ButtonRoot->SetVisibility(bShowActionButtons ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UParadiseSquadDetailWidget::UpdateButtonState(ESquadUIState CurrentState, bool bIsUnitTab, bool bHasPendingSelection)
{
	bool bIsNormal = (CurrentState == ESquadUIState::Normal);

	// 교체 모드 진입 시(인벤토리 클릭 등), 숨겨져 있던 하단 버튼 박스를 강제로 노출
	if (HBox_ButtonRoot && !bIsNormal)
	{
		HBox_ButtonRoot->SetVisibility(ESlateVisibility::Visible);
	}

	if (bIsNormal)
	{
		// [일반 모드] 취소/확인 버튼 숨김
		if (Btn_CancelEquipMode) Btn_CancelEquipMode->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_Confirm)         Btn_Confirm->SetVisibility(ESlateVisibility::Collapsed);

		// 캐릭터/유닛 여부에 따라 장비 교체 버튼 가시성 제어
		if (Btn_SwapCharacter) Btn_SwapCharacter->SetVisibility(ESlateVisibility::Visible);
		if (Btn_SwapEquipment) Btn_SwapEquipment->SetVisibility(bIsUnitTab ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	else
	{
		// [교체 모드] 일반 교체 버튼 숨김
		if (Btn_SwapCharacter) Btn_SwapCharacter->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_SwapEquipment) Btn_SwapEquipment->SetVisibility(ESlateVisibility::Collapsed);

		// 취소/확인 버튼 노출 및 확정 가능 여부에 따른 활성화
		if (Btn_CancelEquipMode) Btn_CancelEquipMode->SetVisibility(ESlateVisibility::Visible);
		if (Btn_Confirm)
		{
			Btn_Confirm->SetVisibility(ESlateVisibility::Visible);
			Btn_Confirm->SetIsEnabled(bHasPendingSelection);
		}
	}
}

void UParadiseSquadDetailWidget::ClearInfo()
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (Text_Name) Text_Name->SetText(FText::GetEmpty());
	if (Text_Desc) Text_Desc->SetText(FText::GetEmpty());
	if (Img_Icon)  Img_Icon->SetVisibility(ESlateVisibility::Collapsed);

	if (Container_EquippedItems) Container_EquippedItems->SetVisibility(ESlateVisibility::Collapsed);
	if (Container_Skill)         Container_Skill->SetVisibility(ESlateVisibility::Collapsed);
	if (HBox_ButtonRoot)         HBox_ButtonRoot->SetVisibility(ESlateVisibility::Collapsed);
}
#pragma endregion 공개 함수

void UParadiseSquadDetailWidget::UpdateEquipmentIcon(EEquipmentSlot InSlot, UImage* TargetImage, const TMap<EEquipmentSlot, FGuid>& EquipmentMap)
{
	// 캡슐화 방어 코드 (대상이 없으면 연산 스킵)
	if (!TargetImage) return;

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	UInventorySystem* InvSys = GI ? GI->GetSubsystem<UInventorySystem>() : nullptr;
	if (!GI || !InvSys) return;

	// 1. 캐릭터가 해당 부위(InSlot)에 장비를 끼고 있는지 확인 🚨 (Slot -> InSlot 변경)
	if (const FGuid* ItemUID = EquipmentMap.Find(InSlot))
	{
		// 2. 인벤토리에서 실제 아이템(UID) 데이터 조회
		if (FOwnedItemData* ItemData = InvSys->GetItemByGUID(*ItemUID))
		{
			UTexture2D* LoadedIcon = nullptr;

			// 3. 무기/방어구 여부에 따라 각각의 에셋 테이블에서 아이콘 로드 (Data-Driven) 🚨 (Slot -> InSlot 변경)
			if (InSlot == EEquipmentSlot::Weapon)
			{
				if (FWeaponAssets* WeaponAsset = GI->GetDataTableRow<FWeaponAssets>(GI->WeaponAssetsDataTable, ItemData->ItemID))
				{
					LoadedIcon = WeaponAsset->Icon.LoadSynchronous();
				}
			}
			else
			{
				if (FArmorAssets* ArmorAsset = GI->GetDataTableRow<FArmorAssets>(GI->ArmorAssetsDataTable, ItemData->ItemID))
				{
					LoadedIcon = ArmorAsset->Icon.LoadSynchronous();
				}
			}

			// 4. 아이콘 적용
			if (LoadedIcon)
			{
				TargetImage->SetBrushFromTexture(LoadedIcon);
				TargetImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				return;
			}
		}
	}

	// 5. 장비를 끼고 있지 않거나 데이터를 찾지 못했다면 폴백 이미지 처리 🚨
	if (DefaultEquipIcon)
	{
		TargetImage->SetBrushFromTexture(DefaultEquipIcon);
		TargetImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		// 기본 아이콘도 없으면 그냥 숨김 (레이아웃 보존)
		TargetImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

#pragma region 핸들러
void UParadiseSquadDetailWidget::HandleSwapChar()
{
	if (OnSwapCharacterClicked.IsBound()) OnSwapCharacterClicked.Broadcast();
}

void UParadiseSquadDetailWidget::HandleSwapEquip()
{
	if (OnSwapEquipmentClicked.IsBound()) OnSwapEquipmentClicked.Broadcast();
}

void UParadiseSquadDetailWidget::HandleCancel()
{
	if (OnCancelClicked.IsBound()) OnCancelClicked.Broadcast();
}

void UParadiseSquadDetailWidget::HandleConfirm()
{
	if (OnConfirmClicked.IsBound()) OnConfirmClicked.Broadcast();
}
#pragma endregion 핸들러