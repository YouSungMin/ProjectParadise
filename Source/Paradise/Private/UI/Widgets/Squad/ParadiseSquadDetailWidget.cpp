// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/ParadiseSquadDetailWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"  
#include "Data/Structs/GrowthStruct.h"

#pragma region 생명주기
void UParadiseSquadDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_SwapCharacter)   Btn_SwapCharacter->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleSwapChar);
	if (Btn_SwapEquipment)   Btn_SwapEquipment->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleSwapEquip);
	if (Btn_CancelEquipMode) Btn_CancelEquipMode->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleCancel);
	if (Btn_Confirm)         Btn_Confirm->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleConfirm);
	// 03/24 판매 버튼 바인드만 해놓은 상태입니다.
	if (Btn_Sell)            Btn_Sell->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleSell);

	SetVisibility(ESlateVisibility::Collapsed);
}

void UParadiseSquadDetailWidget::NativeDestruct()
{
	if (Btn_SwapCharacter)   Btn_SwapCharacter->OnClicked.RemoveAll(this);
	if (Btn_SwapEquipment)   Btn_SwapEquipment->OnClicked.RemoveAll(this);
	if (Btn_CancelEquipMode) Btn_CancelEquipMode->OnClicked.RemoveAll(this);
	if (Btn_Confirm)         Btn_Confirm->OnClicked.RemoveAll(this);
	if (Btn_Sell) Btn_Sell->OnClicked.RemoveAll(this);

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 공개 함수
void UParadiseSquadDetailWidget::ShowInfo(const FSquadItemUIData& InData, ESquadDetailContext InContext)
{
	// 1. 핵심 시스템 캐싱 및 유효성 검사 (최적화)
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [SquadDetail] GameInstance를 찾을 수 없습니다."));
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	UInventorySystem* InvSys = GI->GetSubsystem<UInventorySystem>();

	SetVisibility(ESlateVisibility::Visible);

	// 현재 컨텍스트 캐싱
	CachedContext = InContext;

	// 2. UI 레이아웃 및 텍스트 변수 초기화
	if (Container_EquippedItems) Container_EquippedItems->SetVisibility(ESlateVisibility::Collapsed);
	if (Container_Skill)         Container_Skill->SetVisibility(ESlateVisibility::Collapsed);

	TObjectPtr<UTexture2D> DeterminatedIcon = nullptr; // 최종 결정될 아이콘
	FString FinalStatString = TEXT("");
	FString SkillInfoString = TEXT("스킬 정보가 없습니다.");

	// DT에서 DisplayName을 찾지 못할 경우를 대비한 기본값
	FText FinalDisplayName = InData.Name.IsEmpty() ? FText::FromString(TEXT("-")) : InData.Name;
	bool bShowActionButtons = false;

	// 3. 컨텍스트(Context)에 따른 데이터 로드 및 UI 구성
	switch (InContext)
	{
	case ESquadDetailContext::FormationCharacter:
	case ESquadDetailContext::InventoryCharacter:
	{
		// UI 컨테이너 가시성 설정
		if (Container_EquippedItems) Container_EquippedItems->SetVisibility(InContext == ESquadDetailContext::FormationCharacter ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (Container_Skill) Container_Skill->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		bShowActionButtons = (InContext == ESquadDetailContext::FormationCharacter);

		// 캐릭터는 FaceIcon 로드
		if (FCharacterAssets* CharAsset = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, InData.ID))
		{
			if (!CharAsset->FaceIcon.IsNull()) DeterminatedIcon = CharAsset->FaceIcon.LoadSynchronous();
		}

		// [스탯 연산] 기본 스탯 + 레벨업 성장치 + 장비 스탯 합산
		if (FCharacterStats* CharStat = GI->GetDataTableRow<FCharacterStats>(GI->CharacterStatsDataTable, InData.ID))
		{
			FinalDisplayName = CharStat->DisplayName;

			int32 Level = FMath::Max(1, InData.Level);
			float TotalHP = CharStat->BaseMaxHP + (CharStat->GrowthHPPerLevel * (Level - 1));
			float TotalAtk = CharStat->BaseAttackPower + (CharStat->GrowthAttackPerLevel * (Level - 1));

			if (InvSys)
			{
				if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(InData.ID))
				{
					// 장착 중인 장비 스탯 더하기 로직
					for (const auto& EquipPair : CharData->EquipmentMap)
					{
						FGuid ItemUID = EquipPair.Value;
						if (FOwnedItemData* ItemData = InvSys->GetItemByGUID(ItemUID))
						{
							if (EquipPair.Key == EEquipmentSlot::Weapon)
							{
								// 무기: 공격력만 바로 합산!
								if (FWeaponStats* WpnStat = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, ItemData->ItemID))
								{
									TotalAtk += WpnStat->AttackPower;
								}
							}
							else
							{
								// 방어구/장신구: 체력만 바로 합산!
								if (FArmorStats* ArmStat = GI->GetDataTableRow<FArmorStats>(GI->ArmorStatsDataTable, ItemData->ItemID))
								{
									TotalHP += ArmStat->MaxHP;
								}
							}
						}
					}

					// 편성창일 경우 하단 장비 아이콘 업데이트
					if (InContext == ESquadDetailContext::FormationCharacter)
					{
						UpdateEquipmentIcon(EEquipmentSlot::Weapon, Img_EquipWeapon, CharData->EquipmentMap);
						UpdateEquipmentIcon(EEquipmentSlot::Hat, Img_EquipHelmet, CharData->EquipmentMap);
						UpdateEquipmentIcon(EEquipmentSlot::Armor, Img_EquipChest, CharData->EquipmentMap);
						UpdateEquipmentIcon(EEquipmentSlot::Necklace, Img_EquipAcc1, CharData->EquipmentMap);
						UpdateEquipmentIcon(EEquipmentSlot::Ring, Img_EquipAcc2, CharData->EquipmentMap);
					}
				}
			}

			// [Rich Text 적용] 문자열을 HTML 태그 형식으로 조립
			FinalStatString = FString::Printf(TEXT("<Green>Lv.%d</>\n<Red>체력: %d</> / <Blue>공격력: %d</>"),
				Level, FMath::RoundToInt(TotalHP), FMath::RoundToInt(TotalAtk));

			// [스킬 연동] FActionStats 구조체에서 ActionName 추출
			SkillInfoString = TEXT("고유 스킬: 없음");
			if (!CharStat->UltimateActionHandle.IsNull())
			{
				if (FActionStats* ActionRow = CharStat->UltimateActionHandle.GetRow<FActionStats>(TEXT("UI_SkillNameLookup")))
				{
					SkillInfoString = FString::Printf(TEXT("고유 스킬: %s"), *ActionRow->ActionName.ToString());
				}
			}
		}
	}
	break;

	case ESquadDetailContext::InventoryWeapon:
	{
		// 무기 탭일 때 버튼 영역 활성화
		bShowActionButtons = true;
		if (Container_Skill) Container_Skill->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		// 무기는 일반 Icon 로드
		if (FWeaponAssets* WeaponAsset = GI->GetDataTableRow<FWeaponAssets>(GI->WeaponAssetsDataTable, InData.ID))
		{
			if (!WeaponAsset->Icon.IsNull()) DeterminatedIcon = WeaponAsset->Icon.LoadSynchronous();
		}

		if (FWeaponStats* WpnStat = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, InData.ID))
		{
			FinalDisplayName = WpnStat->DisplayName;

			// 1. 강화 수치 반영 (공격력만 10% 증가 적용 - 기획에 따라 변경 가능)
			float AtkBonus = 1.0f + (InData.Level * 0.1f);
			int32 FinalAtk = FMath::RoundToInt(WpnStat->AttackPower * AtkBonus);

			// 2. 소수점 데이터를 UI용 퍼센트로 변환 (예: 0.15 -> 15%, 1.5 -> 150%)
			int32 CritChance = FMath::RoundToInt(WpnStat->CritRate * 100.0f);
			int32 CritDmg = FMath::RoundToInt(WpnStat->CritDamage * 100.0f);

			// 3. 무기의 모든 스탯을 깔끔하게 표기
			FinalStatString = FString::Printf(TEXT("<Green>강화 +%d</>\n<Blue>공격력: %d</>\n공격 속도: %.2f\n치명타 확률: %d%%\n치명타 피해량: %d%%"),
				InData.Level, FinalAtk, WpnStat->AttackSpeed, CritChance, CritDmg);

			SkillInfoString = TEXT("무기 스킬: 없음");
			if (!WpnStat->SkillActionHandle.IsNull())
			{
				if (FActionStats* ActionRow = WpnStat->SkillActionHandle.GetRow<FActionStats>(TEXT("UI_WeaponSkillNameLookup")))
				{
					SkillInfoString = FString::Printf(TEXT("무기 스킬: %s"), *ActionRow->ActionName.ToString());
				}
			}
		}
	}
	break;

	case ESquadDetailContext::InventoryArmor:
	{
		// 장비 탭일 때 버튼 영역 활성화
		bShowActionButtons = true;
		// 방어구는 일반 Icon 로드
		if (FArmorAssets* ArmorAsset = GI->GetDataTableRow<FArmorAssets>(GI->ArmorAssetsDataTable, InData.ID))
		{
			if (!ArmorAsset->Icon.IsNull()) DeterminatedIcon = ArmorAsset->Icon.LoadSynchronous();
		}

		if (FArmorStats* ArmStat = GI->GetDataTableRow<FArmorStats>(GI->ArmorStatsDataTable, InData.ID))
		{
			FinalDisplayName = ArmStat->DisplayName;

			// 1. 강화 수치 반영 (방어력과 체력, 마나 모두 레벨당 10% 증가 적용)
			float DefBonus = 1.0f + (InData.Level * 0.1f);
			int32 FinalDef = FMath::RoundToInt(ArmStat->DefensePower * DefBonus);
			int32 FinalHP = FMath::RoundToInt(ArmStat->MaxHP * DefBonus);
			int32 FinalMana = FMath::RoundToInt(ArmStat->MaxMana * DefBonus);

			// [Rich Text 적용] 방어구 스탯 표기
			FinalStatString = FString::Printf(TEXT("<Green>강화 +%d</>\n방어력: %d\n<Red>최대 체력: %d</>\n최대 마나: %d"),
				InData.Level, FinalDef, FinalHP, FinalMana);

			SkillInfoString = TEXT("방어구는 고유 스킬이 없습니다.");
		}
	}
	break;

	case ESquadDetailContext::FormationUnit:
		bShowActionButtons = true; // Fallthrough
	case ESquadDetailContext::InventoryUnit:
	{
		// 퍼밀리어(유닛)는 FaceIcon 로드
		if (FFamiliarAssets* UnitAsset = GI->GetDataTableRow<FFamiliarAssets>(GI->FamiliarAssetsDataTable, InData.ID))
		{
			if (!UnitAsset->FaceIcon.IsNull()) DeterminatedIcon = UnitAsset->FaceIcon.LoadSynchronous();
		}

		if (FFamiliarStats* UnitStat = GI->GetDataTableRow<FFamiliarStats>(GI->FamiliarStatsDataTable, InData.ID))
		{
			FinalDisplayName = UnitStat->DisplayName;

			// [Rich Text 적용] 소환수 스탯 표기
			FinalStatString = FString::Printf(TEXT("<Green>소환 코스트: %d</>\n<Red>체력: %d</> / <Blue>공격력: %d</>"),
				UnitStat->SummonCost, FMath::RoundToInt(UnitStat->BaseMaxHP), FMath::RoundToInt(UnitStat->BaseAttackPower));

			SkillInfoString = TEXT("주요 스킬: 없음");
			if (UnitStat->PatternActionHandles.Num() > 0 && !UnitStat->PatternActionHandles[0].IsNull())
			{
				if (FActionStats* ActionRow = UnitStat->PatternActionHandles[0].GetRow<FActionStats>(TEXT("UI_FamiliarSkillNameLookup")))
				{
					SkillInfoString = FString::Printf(TEXT("주요 스킬: %s"), *ActionRow->ActionName.ToString());
				}
			}
		}
	}
	break;
	}

	// 4. 메인 아이콘 최종 렌더링
	if (Img_Icon)
	{
		UTexture2D* FinalIcon = DeterminatedIcon ? DeterminatedIcon : DefaultMainIcon;

		if (FinalIcon)
		{
			Img_Icon->SetBrushFromTexture(FinalIcon);
			Img_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 5. 텍스트 및 버튼 가시성 최종 갱신
	if (Text_Name) Text_Name->SetText(FinalDisplayName);
	if (Text_Desc) Text_Desc->SetText(FText::FromString(FinalStatString));
	if (Text_SkillInfo) Text_SkillInfo->SetText(FText::FromString(SkillInfoString));
	if (HBox_ButtonRoot) HBox_ButtonRoot->SetVisibility(bShowActionButtons ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	bool bIsEquipment = (InContext == ESquadDetailContext::InventoryWeapon || InContext == ESquadDetailContext::InventoryArmor);
	if (Btn_Sell)
	{
		Btn_Sell->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UParadiseSquadDetailWidget::UpdateButtonState(ESquadUIState CurrentState, bool bIsUnitTab, bool bHasPendingSelection)
{
	bool bIsNormal = (CurrentState == ESquadUIState::Normal);
	bool bIsEquipmentContext = (CachedContext == ESquadDetailContext::InventoryWeapon || CachedContext == ESquadDetailContext::InventoryArmor);

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

		// [판매 버튼 제어] 무기/장비 컨텍스트일 때만 판매 버튼 활성화
		if (Btn_Sell) Btn_Sell->SetVisibility(bIsEquipmentContext ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	else
	{
		// [교체 모드] 일반 교체 버튼 숨김
		if (Btn_SwapCharacter) Btn_SwapCharacter->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_SwapEquipment) Btn_SwapEquipment->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_Sell) Btn_Sell->SetVisibility(bIsEquipmentContext ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

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

	if (Btn_Sell) Btn_Sell->SetVisibility(ESlateVisibility::Collapsed);
}
#pragma endregion 공개 함수

void UParadiseSquadDetailWidget::UpdateEquipmentIcon(EEquipmentSlot InSlot, UImage* TargetImage, const TMap<EEquipmentSlot, FGuid>& EquipmentMap)
{
	// 캡슐화 방어 코드 (대상이 없으면 연산 스킵)
	if (!TargetImage) return;

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	UInventorySystem* InvSys = GI ? GI->GetSubsystem<UInventorySystem>() : nullptr;
	if (!GI || !InvSys) return;

	// 1. 캐릭터가 해당 부위(InSlot)에 장비를 끼고 있는지 확인 (Slot -> InSlot 변경)
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

	// 5. [최적화] TMap에서 부위(InSlot)를 키값으로 기본 아이콘을 찾습니다.
	if (const TObjectPtr<UTexture2D>* FoundDefaultIcon = DefaultEquipmentIcons.Find(InSlot))
	{
		if (*FoundDefaultIcon)
		{
			TargetImage->SetBrushFromTexture(*FoundDefaultIcon);
			TargetImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			return; // 성공적으로 렌더링했으면 함수 종료
		}
	}

	// 맵에 해당 부위가 등록되지 않았거나 이미지가 비어있으면 숨김 처리
	TargetImage->SetVisibility(ESlateVisibility::Hidden);
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
void UParadiseSquadDetailWidget::HandleSell()
{
	// 03/24 판매 버튼 눌렸다고 방송만 해놓은 상태입니다.
	if (OnSellClicked.IsBound()) OnSellClicked.Broadcast();
}
#pragma endregion 핸들러