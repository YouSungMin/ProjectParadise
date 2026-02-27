// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/InventorySystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/GrowthSubsystem.h"

// Sets default values for this component's properties
UInventorySystem::UInventorySystem()
{
}

void UInventorySystem::SetCharacterLevelAndExp(FName CharacterID, int32 NewLevel, int32 NewExp)
{
	for (FOwnedCharacterData& CharData : OwnedCharacters)
	{
		if (CharData.CharacterID == CharacterID)
		{
			CharData.Level = NewLevel;
			CharData.CurrentExp = NewExp;
			if (OnInventoryUpdated.IsBound()) OnInventoryUpdated.Broadcast();
			return;
		}
	}
}

void UInventorySystem::InitInventory(const TArray<FOwnedCharacterData>& InHeroes, const TArray<FOwnedFamiliarData>& InFamiliars, const TArray<FOwnedItemData>& InItems)
{
	UParadiseGameInstance* GI = GetParadiseGI();
	if (!GI) return;

	OwnedCharacters.Empty();
	OwnedFamiliars.Empty();
	OwnedItems.Empty();

	//영웅(Character) 로드
	if (InHeroes.Num() > 0)
	{
		for (int i = 0; i < InHeroes.Num(); i++)
		{
			if (GI->IsValidPlayerID(InHeroes[i].CharacterID))
			{
				OwnedCharacters.Add(InHeroes[i]);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Inventory] 영웅 유효성 실패(Asset/Stat 누락): %s"), *InHeroes[i].CharacterID.ToString());
			}
		}
	}

	//퍼밀리어(Familiar) 로드
	if (InFamiliars.Num() > 0)
	{
		for (int i = 0; i < InFamiliars.Num(); i++)
		{
			if (GI->IsValidFamiliarID(InFamiliars[i].FamiliarID))
			{
				OwnedFamiliars.Add(InFamiliars[i]);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Inventory] 퍼밀리어 유효성 실패(Asset/Stat 누락): %s"), *InFamiliars[i].FamiliarID.ToString());
			}
		}
	}

	//아이템(Item) 로드 (무기 or 방어구)
	if (InItems.Num() > 0)
	{
		for (int i = 0; i < InItems.Num(); i++)
		{
			FName ID = InItems[i].ItemID;

			if (GI->IsValidItemID(ID))
			{
				OwnedItems.Add(InItems[i]);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Inventory] 아이템 유효성 실패(Asset/Stat 누락): %s"), *ID.ToString());
			}
		}
	}

	if (OnInventoryUpdated.IsBound())
	{
		OnInventoryUpdated.Broadcast();
	}

	UE_LOG(LogTemp, Log, TEXT("✅ 인벤토리 로드 완료 (영웅:%d, 병사:%d, 아이템:%d)"),
		OwnedCharacters.Num(), OwnedFamiliars.Num(), OwnedItems.Num());
}

void UInventorySystem::AddItem(FName ItemID, int32 Count, int32 EnhancementLvl)
{
	UParadiseGameInstance* GI = GetParadiseGI();
	if (!GI) return;
	if (ItemID.IsNone() || Count <= 0) return;

	bool bExist = GI->IsValidItemID(ItemID);

	// 둘 다 아니면 실패
	if (!bExist)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AddItem] 실패: ID(%s)가 무기/방어구 테이블(Assets & Stats) 쌍에 존재하지 않습니다."), *ItemID.ToString());
		return;
	}

	for (int32 i = 0; i < Count; i++)
	{
		FOwnedItemData NewItem;
		NewItem.ItemUID = FGuid::NewGuid(); // ⭐ 핵심: 고유 ID 발급
		NewItem.ItemID = ItemID;
		NewItem.EnhancementLevel = EnhancementLvl;
		NewItem.Quantity = 1;

		OwnedItems.Add(NewItem);

		UE_LOG(LogTemp, Log, TEXT("✨ [AddItem] 장비 개별 획득: %s (UID: %s)"),
			*ItemID.ToString(), *NewItem.ItemUID.ToString());
	}

	if (OnInventoryUpdated.IsBound()) OnInventoryUpdated.Broadcast();
}

void UInventorySystem::AddCharacter(FName CharacterID)
{
	UParadiseGameInstance* GI = GetParadiseGI();
	if (!GI) return;

	//이미 보유 중인 캐릭터인지 먼저 확인
	if (HasCharacter(CharacterID))
	{
		// 중복 획득 처리는 UGrowthSubsystem에서 처리
		if (UGrowthSubsystem* GrowthSys = GI->GetSubsystem<UGrowthSubsystem>())
		{
			GrowthSys->HandleDuplicateCharacter(CharacterID);
		}
		return;
	}

	//최초 획득시 
	if (!GI->IsValidPlayerID(CharacterID)) return;

	FOwnedCharacterData NewCharacter;
	NewCharacter.CharacterUID = FGuid::NewGuid();
	NewCharacter.CharacterID = CharacterID;
	NewCharacter.Level = 1;
	NewCharacter.AwakeningLevel = 0; // 초기 각성 레벨
	NewCharacter.AwakeningPieces = 0;

	OwnedCharacters.Add(NewCharacter);

	if (OnInventoryUpdated.IsBound())
	{
		OnInventoryUpdated.Broadcast();
	}
}

void UInventorySystem::AddFamiliar(FName FamiliarID)
{
	UParadiseGameInstance* GI = GetParadiseGI();
	if (!GI) return;
	if (FamiliarID.IsNone()) return;

	bool bExist = GI->IsValidFamiliarID(FamiliarID);
	if (!bExist)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AddFamiliar] 실패: ID(%s)가 퍼밀리어 Assets 혹은 Stats 테이블에 없습니다."), *FamiliarID.ToString());
		return;
	}

	// [수정 후] 무조건 새로운 병사로 추가
	FOwnedFamiliarData NewFamiliar;
	NewFamiliar.FamiliarUID = FGuid::NewGuid(); // ⭐ 핵심
	NewFamiliar.FamiliarID = FamiliarID;
	NewFamiliar.Level = 1;
	NewFamiliar.Quantity = 1;

	OwnedFamiliars.Add(NewFamiliar);

	UE_LOG(LogTemp, Log, TEXT("🥚 [AddFamiliar] 병사 영입: %s (UID: %s)"),
		*FamiliarID.ToString(), *NewFamiliar.FamiliarUID.ToString());

	if (OnInventoryUpdated.IsBound()) OnInventoryUpdated.Broadcast();
}

void UInventorySystem::AddAwakeningPiece(FName CharacterID, int32 Count)
{
	for (FOwnedCharacterData& CharData : OwnedCharacters)
	{
		if (CharData.CharacterID == CharacterID)
		{
			CharData.AwakeningPieces += Count;
			if (OnInventoryUpdated.IsBound()) OnInventoryUpdated.Broadcast();
			return;
		}
	}
}

bool UInventorySystem::RemoveObjectByGUID(FGuid TargetGUID, int32 Count)
{
	if (!TargetGUID.IsValid()) return false;

	// 1. [아이템] 배열 검색
	for (int32 i = 0; i < OwnedItems.Num(); ++i)
	{
		if (OwnedItems[i].ItemUID == TargetGUID)
		{
			// 장비 로직: 수량 차감
			if (OwnedItems[i].Quantity > Count)
			{
				OwnedItems[i].Quantity -= Count;
				UE_LOG(LogTemp, Log, TEXT("📉 아이템 수량 감소: %s"), *TargetGUID.ToString());
			}
			else
			{
				OwnedItems.RemoveAt(i);
				UE_LOG(LogTemp, Log, TEXT("🗑️ 아이템 삭제 완료: %s"), *TargetGUID.ToString());
			}

			if (OnInventoryUpdated.IsBound()) OnInventoryUpdated.Broadcast();
			return true; // 찾았으니 리턴
		}
	}

	// 2. [퍼밀리어] 배열 검색
	for (int32 i = 0; i < OwnedFamiliars.Num(); ++i)
	{
		if (OwnedFamiliars[i].FamiliarUID == TargetGUID)
		{
			// 병사는 개별 관리이므로 즉시 삭제
			OwnedFamiliars.RemoveAt(i);
			UE_LOG(LogTemp, Log, TEXT("🗑️ 퍼밀리어 삭제 완료: %s"), *TargetGUID.ToString());

			if (OnInventoryUpdated.IsBound()) OnInventoryUpdated.Broadcast();
			return true;
		}
	}

	// 3. [영웅] 배열 검색
	for (int32 i = 0; i < OwnedCharacters.Num(); ++i)
	{
		if (OwnedCharacters[i].CharacterUID == TargetGUID)
		{
			OwnedCharacters.RemoveAt(i);
			UE_LOG(LogTemp, Log, TEXT("👋 영웅 삭제(해고) 완료: %s"), *TargetGUID.ToString());

			if (OnInventoryUpdated.IsBound()) OnInventoryUpdated.Broadcast();
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("❌ [Remove] 해당 GUID를 가진 객체를 찾을 수 없습니다: %s"), *TargetGUID.ToString());
	return false;
}

int32 UInventorySystem::GetItemQuantity(FName ItemID) const
{
	if (ItemID.IsNone()) return 0;
	int32 TotalCount = 0;

	for (const auto& Item : OwnedItems)
	{
		if (Item.ItemID == ItemID)
		{
			TotalCount += Item.Quantity; // 장비는 주로 1이지만, 합산 로직 유지
		}
	}
	return TotalCount;
}


FOwnedItemData* UInventorySystem::GetItemByGUID(FGuid TargetUID)
{
	if (!TargetUID.IsValid()) return nullptr;

	for (auto& Item : OwnedItems)
	{
		if (Item.ItemUID == TargetUID)
		{
			return &Item;
		}
	}
	return nullptr;
}

bool UInventorySystem::HasCharacter(FName CharacterID) const
{
	for (const auto& Hero : OwnedCharacters)
	{
		if (Hero.CharacterID == CharacterID) return true;
	}
	return false;
}

const FOwnedCharacterData* UInventorySystem::GetCharacterDataByID(FName CharacterID) const
{
	// 보유한 캐릭터 배열을 순회하며 ID가 일치하는지 확인
	for (const FOwnedCharacterData& CharData : OwnedCharacters)
	{
		if (CharData.CharacterID == CharacterID)
		{
			// 찾았으면 해당 데이터의 주소값 반환
			return &CharData;
		}
	}
	return nullptr;
}

UParadiseGameInstance* UInventorySystem::GetParadiseGI() const
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetOuter()))
	{
		return GI;
	}
	return nullptr;
}


EEquipmentSlot UInventorySystem::FindEquipmentSlot(FName ItemID) const
{
	if (ItemID.IsNone()) return EEquipmentSlot::Unknown;

	UParadiseGameInstance* GI = GetParadiseGI();
	if (!GI) return EEquipmentSlot::Unknown;

	//무기 테이블 확인
	if (GI->GetDataTableRow<FWeaponAssets>(GI->WeaponAssetsDataTable, ItemID))
	{
		return EEquipmentSlot::Weapon;
	}

	//방어구 테이블 확인
	if (FArmorAssets* ArmorRow = GI->GetDataTableRow<FArmorAssets>(GI->ArmorAssetsDataTable, ItemID))
	{
		// 태그 비교 로직
		const FGameplayTag& Tag = ArmorRow->ArmorTag;

		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag("Item.Type.Armor.Helmet"))) return EEquipmentSlot::Helmet;
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag("Item.Type.Armor.Chest")))  return EEquipmentSlot::Chest;
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag("Item.Type.Armor.Gloves"))) return EEquipmentSlot::Gloves;
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag("Item.Type.Armor.Boots")))  return EEquipmentSlot::Boots;

		// 매칭되는 태그가 없으면 경고
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [FindSlot] 알 수 없는 방어구 태그: %s"), *Tag.ToString());
	}

	return EEquipmentSlot::Unknown;
}

void UInventorySystem::EquipItemToCharacter(FGuid CharacterUID, FGuid ItemUID)
{
	//인벤토리에 실제 아이템이 있는지 유효성 검사
	FOwnedItemData* ItemData = GetItemByGUID(ItemUID);
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ 인벤토리에 없는 아이템 GUID입니다."));
		return;
	}

	//대상 캐릭터 유효성 검사 (GUID 검사)
	FOwnedCharacterData* TargetChar = nullptr;
	for (auto& Char : OwnedCharacters)
	{
		if (Char.CharacterUID == CharacterUID)
		{
			TargetChar = &Char;
			break;
		}
	}

	if (!TargetChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ 보유하지 않은 캐릭터 UID입니다: %s"), *CharacterUID.ToString());
		return;
	}

	//장비 아이템의 슬롯 판별
	EEquipmentSlot TargetSlot = FindEquipmentSlot(ItemData->ItemID);
	if (TargetSlot == EEquipmentSlot::Unknown) return;


	//다른 캐릭터가 장착 중일 경우
	if (ItemData->EquippedCharacterUID.IsValid() && ItemData->EquippedCharacterUID != CharacterUID)
	{
		// 전 주인(캐릭터)의 데이터 찾기
		for (auto& OldOwner : OwnedCharacters)
		{
			if (OldOwner.CharacterUID == ItemData->EquippedCharacterUID)
			{
				// 전 주인의 장비창(EquipmentMap)에서 이 아이템을 제거
				for (auto It = OldOwner.EquipmentMap.CreateIterator(); It; ++It)
				{
					if (It.Value() == ItemUID)
					{
						It.RemoveCurrent();
						UE_LOG(LogTemp, Warning, TEXT("🔄 [%s]가 끼고 있던 장비를 교체합니다!"), *OldOwner.CharacterID.ToString());
						break;
					}
				}
				break;
			}
		}
	}

	//타겟 캐릭터가 이전에 장착하고있던 장비가 있을경우 
	if (TargetChar->EquipmentMap.Contains(TargetSlot))
	{
		FGuid OldItemUID = TargetChar->EquipmentMap[TargetSlot];
		if (FOwnedItemData* OldItem = GetItemByGUID(OldItemUID))
		{
			// 벗겨지는 옛날 장비의 '주인 UID'를 빈 값으로 초기화 (해제 상태로 만듦)
			OldItem->EquippedCharacterUID = FGuid();
		}
	}


	//덮어쓰기
	TargetChar->EquipmentMap.Add(TargetSlot, ItemUID);
	ItemData->EquippedCharacterUID = CharacterUID;
	//장비 변경 델리게이트 발송
	if (OnInventoryUpdated.IsBound())
	{
		OnInventoryUpdated.Broadcast();
	}

	UE_LOG(LogTemp, Log, TEXT("⚔️ [%s] 캐릭터에게 장비 장착 완료: %s"), *CharacterUID.ToString(), *ItemUID.ToString());
}

void UInventorySystem::UnEquipItemFromCharacter(FGuid CharacterUID, EEquipmentSlot Slot)
{
	for (auto& Char : OwnedCharacters)
	{
		if (Char.CharacterUID == CharacterUID)
		{
			FGuid ItemUIDToRemove = Char.EquipmentMap[Slot];

			//아이템 데이터에서 오너 정보를 빈 값으로 초기화
			if (FOwnedItemData* ItemData = GetItemByGUID(ItemUIDToRemove))
			{
				ItemData->EquippedCharacterUID = FGuid();
			}

			Char.EquipmentMap.Remove(Slot);

			if (OnInventoryUpdated.IsBound())
			{
				OnInventoryUpdated.Broadcast();
			}
			UE_LOG(LogTemp, Log, TEXT("🛡️ [%s] 캐릭터의 슬롯[%d] 장비 해제 완료"), *CharacterUID.ToString(), (int32)Slot);
			return;
		}
	}
}