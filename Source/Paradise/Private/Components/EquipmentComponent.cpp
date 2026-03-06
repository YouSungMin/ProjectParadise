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

	CalculateActiveSetBonuses();
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
	const TArray<EEquipmentSlot> VisualSlots = {
		EEquipmentSlot::Weapon,
		EEquipmentSlot::Hat,
	};

	for (EEquipmentSlot Slot : VisualSlots)
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

TMap<FName, int32> UEquipmentComponent::CalculateActiveSetBonuses() const
{
	TMap<FName, int32> SetCounts;

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return SetCounts;

	UE_LOG(LogTemp, Warning, TEXT("=== 🔄 세트 효과 계산 시작 ==="));
	// 컴포넌트 본인이 들고 있는 EquippedItems 순회
	for (const auto& Pair : EquippedItems)
	{
		EEquipmentSlot Slot = Pair.Key;
		FName ItemID = GetEquippedItemID(Slot);

		if (ItemID.IsNone()) continue;

		FName SetID = NAME_None;

		// 1. 무기 테이블 조회
		if (GI->WeaponStatsDataTable)
		{
			if (FWeaponStats* WeaponStat = GI->WeaponStatsDataTable->FindRow<FWeaponStats>(ItemID, TEXT("")))
			{
				SetID = WeaponStat->SetID;
			}
		}

		// 2. 방어구 테이블 조회 (무기가 아니었다면)
		if (SetID.IsNone() && GI->ArmorStatsDataTable)
		{
			if (FArmorStats* ArmorStat = GI->ArmorStatsDataTable->FindRow<FArmorStats>(ItemID, TEXT("")))
			{
				SetID = ArmorStat->SetID;
			}
		}

		// 세트 ID가 존재하면 카운트 +1
		if (!SetID.IsNone())
		{
			int32& Count = SetCounts.FindOrAdd(SetID);
			Count++;
			UE_LOG(LogTemp, Log, TEXT("✅ 장비 인식: [%s] -> 적용 세트: [%s] (누적 %d개)"), *ItemID.ToString(), *SetID.ToString(), Count);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("=== 📊 최종 활성화된 세트 효과 목록 ==="));
	for (const auto& Result : SetCounts)
	{
		UE_LOG(LogTemp, Warning, TEXT(" - 세트 ID: [%s] | 장착 부위 개수: %d"), *Result.Key.ToString(), Result.Value);
	}

	return SetCounts;
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


