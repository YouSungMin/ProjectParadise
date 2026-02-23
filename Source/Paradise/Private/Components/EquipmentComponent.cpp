// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/EquipmentComponent.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/Player/PlayerData.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"

// Sets default values for this component's properties
UEquipmentComponent::UEquipmentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::TestEquip(APlayerBase* Char, EEquipmentSlot Slot, FName ItemID)
{
	SetEquipmentMesh(Char, Slot, ItemID);
}


void UEquipmentComponent::TestEquippedItem(EEquipmentSlot Slot, FName ItemID)
{
	UInventorySystem* InvSys = GetInventorySystem();
	if (!InvSys)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [TestEquippedItem] 인벤토리 시스템을 찾을 수 없습니다."));
		return;
	}

	if (ItemID.IsNone()) return;

	APlayerData* OwnerData = Cast<APlayerData>(GetOwner());
	if (!OwnerData) return;

	FName MyHeroID = OwnerData->CharacterID;
	FGuid MyCharUID;

	for (const FOwnedCharacterData& CharData : InvSys->GetOwnedCharacters())
	{
		if (CharData.CharacterID == MyHeroID)
		{
			MyCharUID = CharData.CharacterUID;
			break;
		}
	}

	//캐릭터가 없으면 생성해서라도 UID 확보
	if (!MyCharUID.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [Debug] 캐릭터가 없어 강제 생성합니다: %s"), *MyHeroID.ToString());
		InvSys->AddCharacter(MyHeroID);

		// 다시 검색
		for (const FOwnedCharacterData& CharData : InvSys->GetOwnedCharacters())
		{
			if (CharData.CharacterID == MyHeroID)
			{
				MyCharUID = CharData.CharacterUID;
				break;
			}
		}
	}

	//아이템 UID 찾기
	FGuid TargetItemUID;

	// 인벤토리에 해당 ID의 아이템이 있는지 검색
	const TArray<FOwnedItemData>& Items = InvSys->GetOwnedItems();
	for (const FOwnedItemData& Item : Items)
	{
		if (Item.ItemID == ItemID)
		{
			TargetItemUID = Item.ItemUID;
			break; // 하나라도 있으면 그거 씀
		}
	}

	// 디버그코드 이므로 강제로 인벤토리추가 
	if (!TargetItemUID.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [Debug] 인벤토리에 아이템이 없어 강제 생성합니다: %s"), *ItemID.ToString());
		InvSys->AddItem(ItemID, 1);

		// 방금 만든거 다시 찾기
		const TArray<FOwnedItemData>& NewItems = InvSys->GetOwnedItems();
		for (int32 i = NewItems.Num() - 1; i >= 0; --i)
		{
			if (NewItems[i].ItemID == ItemID)
			{
				TargetItemUID = NewItems[i].ItemUID;
				break;
			}
		}
	}

	if (!TargetItemUID.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [Debug] 아이템 생성 실패 (데이터 테이블에 없는 ID일 수 있음): %s"), *ItemID.ToString());
		return;
	}

	//위에서 강제로 만들었으므로 무조건 통과
	InvSys->EquipItemToCharacter(MyCharUID, TargetItemUID);

	//비주얼 강제 업데이트
	APlayerBase* VisualTarget =  nullptr;

	// 타겟이 명시되지 않았다면 PlayerData가 알고 있는 현재 아바타 사용
	if (OwnerData->CurrentAvatar.IsValid())
	{
		VisualTarget = Cast<APlayerBase>(OwnerData->CurrentAvatar.Get());
	}

	if (VisualTarget)
	{
		for (const FOwnedCharacterData& CharData : InvSys->GetOwnedCharacters())
		{
			if (CharData.CharacterUID == MyCharUID)
			{
				InitializeEquipment(CharData.EquipmentMap);
				
				UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance());
				if (!GI)
				{
					UE_LOG(LogTemp, Error, TEXT("❌ [PlayerData] GameInstance 없음!"));
					return;
				}
				FWeaponAssets* WeaponAssets = GI->GetDataTableRow<FWeaponAssets>(GI->WeaponAssetsDataTable, ItemID);
				OwnerData->InitializeWeaponAbilities(WeaponAssets);
				UE_LOG(LogTemp, Log, TEXT("💪 [Debug] %s 장착 및 비주얼 갱신 완료!"), *ItemID.ToString());
				break;
			}
		}
	}
}


void UEquipmentComponent::InitializeEquipment(const TMap<EEquipmentSlot, FGuid>& InEquipmentMap)
{
	UInventorySystem* InvSys = GetInventorySystem();
	if (!InvSys) {
		UE_LOG(LogTemp, Error, TEXT("[UEquipmentComponent] 인벤토리 시스템이 존재하지 않습니다."));
		return;
	}

	//GameInstance가 건네준 최신 장비 데이터(맵)로 내 캐시를 덮어씌움
	EquippedItems = InEquipmentMap;

	//외형 업데이트 실행
	//EquipmentComponent가 현재 Avatar를 찾아 갱신
	if (APlayerData* Soul = Cast<APlayerData>(GetOwner()))
	{
		if (Soul->CurrentAvatar.IsValid())
		{
			if (APlayerBase* Avatar = Cast<APlayerBase>(Soul->CurrentAvatar.Get()))
			{
				UpdateVisuals(Avatar);
			}
		}
	}
	ApplyEquipmentStats();
}

void UEquipmentComponent::ApplyEquipmentStats()
{
	APlayerData* Soul = Cast<APlayerData>(GetOwner());
	if (!Soul) return;

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	UBaseAttributeSet* AttrSet = Soul->GetAttributeSet();
	if (!AttrSet) return;

	// 인게임 장비 교체가 없으므로, 초기화(리셋) 로직은 필요 없습니다!
	// 영혼(PlayerData)이 생성되면서 방금 세팅된 기본 스탯 위에 장비 스탯을 그대로 얹어주기만 합니다.

	for (const auto& Pair : EquippedItems)
	{
		FName ItemID = GetEquippedItemID(Pair.Key);
		if (ItemID.IsNone()) continue;

		//무기
		if (Pair.Key == EEquipmentSlot::Weapon)
		{
			if (FWeaponStats* WStat = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, ItemID))
			{
				// 기본 스탯 + 무기 스탯
				AttrSet->SetAttackPower(AttrSet->GetAttackPower() + WStat->AttackPower);
				// AttrSet->SetAttackRange(AttrSet->GetAttackRange() + WStat->AttackRange);
				AttrSet->SetAttackSpeed(AttrSet->GetAttackSpeed() + WStat->AttackSpeed);
				// AttrSet->SetCooldown(AttrSet->GetCooldown() + WStat->Cooldown);
				AttrSet->SetCritDamage(AttrSet->GetCritDamage() + WStat->CritDamage);
				AttrSet->SetCritRate(AttrSet->GetCritRate() + WStat->CritRate);
			}
		}
		//방어구
		else
		{
			if (FArmorStats* AStat = GI->GetDataTableRow<FArmorStats>(GI->ArmorStatsDataTable, ItemID))
			{
				// 기본 스탯 + 방어구 스탯
				AttrSet->SetMaxMana(AttrSet->GetMaxMana()+AStat->MaxMana);
				AttrSet->SetDefense(AttrSet->GetDefense() + AStat->DefensePower); 
				AttrSet->SetMaxHealth(AttrSet->GetMaxHealth() + AStat->MaxHP);
			}
		}
	}

	// 장비로 인해 최대 체력(MaxHP)이 늘어났으므로, 현재 체력(HP)도 꽉 채워서 전투를 시작하도록 보정
	AttrSet->SetHealth(AttrSet->GetMaxHealth());

	UE_LOG(LogTemp, Log, TEXT("💪 [Equipment] 장비 스탯 최종 세팅 완료 (Attack: %.1f, Defense: %.1f, HP: %.1f)"),
		AttrSet->GetAttackPower(), AttrSet->GetDefense(), AttrSet->GetMaxHealth());
}



FName UEquipmentComponent::GetEquippedItemID(EEquipmentSlot Slot) const
{
	UInventorySystem* InvSys = GetInventorySystem();
	if (!InvSys) {
		UE_LOG(LogTemp, Error, TEXT("[UEquipmentComponent] 인벤토리 시스템이 존재하지 않습니다."));
		return NAME_None;
	}

    //해당 슬롯에 GUID가 없으면 None
    if (!EquippedItems.Contains(Slot)) return NAME_None;

    FGuid TargetUID = EquippedItems[Slot];

    //인벤토리에게 물어봐서 데이터 가져오기
    if (FOwnedItemData* ItemData = InvSys->GetItemByGUID(TargetUID))
    {
        //인벤토리에서 찾아서 FName 반환
        return ItemData->ItemID;
    }

    return NAME_None;
}

bool UEquipmentComponent::GetEquippedItemData(EEquipmentSlot Slot, FOwnedItemData& OutData) const
{
	UInventorySystem* InvSys = GetInventorySystem();
	if (!InvSys) {
		UE_LOG(LogTemp, Error, TEXT("[UEquipmentComponent] 인벤토리 시스템이 존재하지 않습니다."));
		return false;
	}
    

    if (const FGuid* FoundGUID = EquippedItems.Find(Slot))
    {
        if (FOwnedItemData* RealData = InvSys->GetItemByGUID(*FoundGUID))
        {
            OutData = *RealData;
            return true;
        }
    }
    return false;
}

UInventorySystem* UEquipmentComponent::GetInventorySystem() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (UGameInstance* GI = World->GetGameInstance())
	{
		return GI->GetSubsystem<UInventorySystem>();
	}
	return nullptr;
}

void UEquipmentComponent::UpdateVisuals(APlayerBase* TargetCharacter)
{
	//타겟이 없으면 컴포넌트 소유자를 사용
	APlayerBase* Char = TargetCharacter ? TargetCharacter : Cast<APlayerBase>(GetOwner());
	if (!Char) return;

	UE_LOG(LogTemp, Log, TEXT("🎨 [Visual] 캐릭터 외형 업데이트 시작... (Optimized)"));

	//무기(Weapon)를 포함한 모든 슬롯을 하나의 배열로 관리
	const TArray<EEquipmentSlot> AllSlots = {
		EEquipmentSlot::Weapon,
		EEquipmentSlot::Helmet,
		EEquipmentSlot::Chest,
		EEquipmentSlot::Gloves,
		EEquipmentSlot::Boots
	};

	for (EEquipmentSlot Slot : AllSlots)
	{
		FOwnedItemData ItemData;

		//해당 슬롯에 장착된 아이템 데이터 조회 (GUID -> ItemData)
		if (GetEquippedItemData(Slot, ItemData))
		{
			//장착 상태: 통합 함수 호출 (무기/방어구 자동 분기 처리)
			SetEquipmentMesh(Char, Slot, ItemData.ItemID);
		}
		else
		{
			//미장착 상태: None 전달 -> 메쉬 비우기 & 숨김 처리
			SetEquipmentMesh(Char, Slot, NAME_None);
		}
	}
}

void UEquipmentComponent::SetEquipmentMesh(APlayerBase* Char, EEquipmentSlot Slot, FName ItemID)
{
	if (!Char) return;

	//해당 슬롯의 메쉬 컴포넌트 가져오기
	USkeletalMeshComponent* TargetMeshComp = Char->GetArmorComponent(Slot);
	if (!TargetMeshComp)
	{
		// 무기가 없거나 해당 슬롯이 없는 경우
		return;
	}

	//장착 해제 (ItemID가 None인 경우)
	if (ItemID.IsNone())
	{
		TargetMeshComp->SetSkeletalMesh(nullptr);
		TargetMeshComp->SetLeaderPoseComponent(nullptr); // 리더 포즈 해제
		TargetMeshComp->SetHiddenInGame(true); // 안 보이게 숨김 (최적화)
		return;
	}

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	//데이터 테이블 조회 및 에셋 로드
	USkeletalMesh* LoadedMesh = nullptr;
	FName SocketName = NAME_None;

	//무기인 경우
	if (Slot == EEquipmentSlot::Weapon)
	{
		if (FWeaponAssets* WeaponData = GI->GetDataTableRow<FWeaponAssets>(GI->WeaponAssetsDataTable, ItemID))
		{
			LoadedMesh = WeaponData->ItemMesh.LoadSynchronous();
			SocketName = WeaponData->AttachmentSocket; // 무기는 반드시 소켓이 있어야 함
		}
	}
	//방어구인 경우
	else
	{
		if (FArmorAssets* ArmorData = GI->GetDataTableRow<FArmorAssets>(GI->ArmorAssetsDataTable, ItemID))
		{
			LoadedMesh = ArmorData->ItemMesh.LoadSynchronous();
			SocketName = ArmorData->AttachmentSocket; // 방어구는 소켓이 없을 수도 있음(None)
		}
	}

	if (!LoadedMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [Visual] 메쉬 로드 실패: %s"), *ItemID.ToString());
		return;
	}

	//메쉬 적용 및 보이기
	TargetMeshComp->SetSkeletalMesh(LoadedMesh);
	TargetMeshComp->SetHiddenInGame(false);

	// 부착 로직 (소켓 부착 vs 리더 포즈)

	//무기거나, 어깨보호구 같은 소켓 부착형 방어구
	if (!SocketName.IsNone())
	{
		//리더 포즈 해제 (소켓에 붙을 땐 리더 포즈 쓰면 안 됨)
		TargetMeshComp->SetLeaderPoseComponent(nullptr);

		//소켓 존재 확인 후 부착
		if (Char->GetMesh()->DoesSocketExist(SocketName))
		{
			TargetMeshComp->AttachToComponent(
				Char->GetMesh(),
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				SocketName
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ [Visual] 소켓 없음: %s. 기본값(hand_r) 사용 시도."), *SocketName.ToString());
			TargetMeshComp->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("hand_r"));
		}
	}
	//소켓 이름이 없다? -> 일반 방어구 (몸에 딱 붙는 옷)
	else
	{
		// 무기는 소켓이 없을 수 없으므로, 방어구만 여기로 옴
		TargetMeshComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		TargetMeshComp->SetupAttachment(Char->GetMesh());
		TargetMeshComp->SetLeaderPoseComponent(Char->GetMesh()); // 애니메이션 동기화
	}

	UE_LOG(LogTemp, Log, TEXT("⚔️🛡️ [Visual] 장비 적용 완료: %s (Slot: %d)"), *ItemID.ToString(), (int32)Slot);
}


