// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/InventorySystem.h"
#include "Framework/Core/ParadiseGameInstance.h"

// Sets default values for this component's properties
UInventorySystem::UInventorySystem()
{
}

void UInventorySystem::AddCharacterExp(FName CharacterID, int32 ExpAmount)
{
	if (CharacterID.IsNone()) return;

	//내 인벤토리에서 캐릭터 데이터 찾기
	FOwnedCharacterData* TargetCharacterData = nullptr;
	for (FOwnedCharacterData& CharData : OwnedCharacters)
	{
		if (CharData.CharacterID == CharacterID)
		{
			TargetCharacterData = &CharData;
			break;
		}
	}

	//획득한 경험치를 전부 더함
	TargetCharacterData->CurrentExp += ExpAmount;
	UE_LOG(LogTemp, Log, TEXT("✨ [%s] 경험치 획득: +%d (총 누적: %d)"), *CharacterID.ToString(), ExpAmount, TargetCharacterData->CurrentExp);

	// 게임 인스턴스 가져오기
	UParadiseGameInstance* GI = GetParadiseGI();
	if (!GI || !GI->CharacterLevelUpDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ CharacterLevelUpDataTable이 게임 인스턴스에 설정되지 않았습니다!"));
		return;
	}

	//데이터 테이블 기반 레벨업 루프
	while (true)
	{
		//다음 레벨 의 RowName 데이터 테이블 찾기
		int32 NextLevel = TargetCharacterData->Level + 1;
		FName RowName = FName(*FString::FromInt(NextLevel));

		//테이블에서 다음 레벨 데이터 조회
		FCharacterLevelUpData* LevelData = GI->GetDataTableRow<FCharacterLevelUpData>(GI->CharacterLevelUpDataTable, RowName);

		//만렙 이후 (데이터없을시)
		if (!LevelData)
		{
			UE_LOG(LogTemp, Warning, TEXT("👑 [%s] 최대 레벨 도달! (Lv.%d)"), *CharacterID.ToString(), TargetCharacterData->Level);
			TargetCharacterData->CurrentExp = 0; // 초과 경험치 증발 처리
			break;
		}

		//경험치가 충분하다면 레벨업 진행
		if (TargetCharacterData->CurrentExp >= LevelData->RequiredExp)
		{
			TargetCharacterData->CurrentExp -= LevelData->RequiredExp;
			TargetCharacterData->Level++;

			UE_LOG(LogTemp, Warning, TEXT("🎉 [%s] 레벨 업! (Lv.%d -> %d)"), *CharacterID.ToString(), TargetCharacterData->Level - 1, TargetCharacterData->Level);

			// TODO : 파티클 시스템이나 레벨업 사운드를 재생하는 이벤트
		}
		else
		{
			// 경험치가 모자라면 루프 종료
			break;
		}
	}

	//UI 델리게이트 발송
	if (OnInventoryUpdated.IsBound())
	{
		OnInventoryUpdated.Broadcast();
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
	if (CharacterID.IsNone()) return;

	bool bExist = GI->IsValidPlayerID(CharacterID);
	if (!bExist)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AddCharacter] 실패: ID(%s)가 캐릭터 Assets 혹은 Stats 테이블에 없습니다."), *CharacterID.ToString());
		return;
	}

	for (int i = 0; i < OwnedCharacters.Num(); i++)
	{
		if (OwnedCharacters[i].CharacterID == CharacterID)
		{
			//중복 캐릭터 획득시 //돌파 재화추가
			if (OwnedCharacters[i].AwakeningPieces < 6)
			{
				OwnedCharacters[i].AwakeningPieces++;

				UE_LOG(LogTemp, Warning, TEXT("✨ [%s] 중복 획득! 영웅의 돌파 조각을 얻었습니다. (현재: %d / 6)"), *CharacterID.ToString(), OwnedCharacters[i].AwakeningPieces);

				// UI 갱신
				if (OnInventoryUpdated.IsBound()) OnInventoryUpdated.Broadcast();
			}
			else
			{
				// 🌟 이미 최대 돌파(6돌) 재화를 다 모았는데 또 뽑았을 때의 처리
				UE_LOG(LogTemp, Warning, TEXT("👑 [%s] 이미 최대 돌파 조각을 보유 중입니다!"), *CharacterID.ToString());

				// [TODO/기획] 마일리지나 범용 재화로 환급해주는 로직을 넣으면 좋습니다.
				// 예시: EconomySys->AddCurrency(ECurrencyType::Mileage, 50);
			}

			return;
		}
	}

	FOwnedCharacterData NewCharacter;
	NewCharacter.CharacterUID = FGuid::NewGuid();
	NewCharacter.CharacterID = CharacterID;
	NewCharacter.Level = 1;
	NewCharacter.AwakeningLevel = 1;

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

	//덮어쓰기
	TargetChar->EquipmentMap.Add(TargetSlot, ItemUID);

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