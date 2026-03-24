// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/SquadSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/ParadiseSaveGame.h"
#include "Framework/System/InventorySystem.h"
#include "Data/Structs/InventoryStruct.h"
#include "Paradise/Paradise.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"


void USquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 3인 플레이어 스쿼드 배열을 NAME_None으로 초기화 (크기 3 고정)
	SelectedPlayerSquadIDs.Init(NAME_None, 3);
	// 5인 퍼밀리어 스쿼드 배열을 NAME_None으로 초기화 (크기 5 고정)
	SelectedFamiliarSquadIDs.Init(NAME_None, 5);


	UE_LOG(LogParadiseSquad, Warning, TEXT("❌ [SquadSubsystem] Initialize 실행"));
	// TODO: 게임 시작 시 저장된 스쿼드 정보가 있다면 여기서 불러옵니다.
	// LoadSquadData();
}

void USquadSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void USquadSubsystem::SetFamiliarToSlot(int32 SlotIndex, FName NewFamiliarID)
{
	//유효한 슬롯인지 확인 
	if (!SelectedFamiliarSquadIDs.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogParadiseSquad, Warning, TEXT("❌ [SquadSubsystem] 유효하지 않은 퍼밀리어 슬롯 인덱스입니다: %d (0~4만 가능)"), SlotIndex);
		return;
	}

	//이미 해당 슬롯에 같은 퍼밀리어가 있다면 무시 (최적화)
	if (SelectedFamiliarSquadIDs[SlotIndex] == NewFamiliarID)
	{
		return;
	}

	//전역 데이터 유효성 검사
	if (!NewFamiliarID.IsNone())
	{
		UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
		if (!GI) return;

		// A. 게임에 존재하는 진짜 퍼밀리어 ID인지 확인
		if (!GI->IsValidFamiliarID(NewFamiliarID))
		{
			UE_LOG(LogParadiseSquad, Error, TEXT("❌ [SquadSubsystem] 편성 실패! '%s'는 유효하지 않은 퍼밀리어 ID입니다."), *NewFamiliarID.ToString());
			return;
		}
	}

	//중복 편성 시 (자리 스왑 로직)
	if (!NewFamiliarID.IsNone() && IsFamiliarAlreadyAssigned(NewFamiliarID))
	{
		int32 OldIndex = SelectedFamiliarSquadIDs.Find(NewFamiliarID);

		if (OldIndex != INDEX_NONE)
		{
			// 현재 넣으려는 타겟 슬롯에 있던 퍼밀리어를 내 예전 자리로 보냄
			FName CharacterToSwap = SelectedFamiliarSquadIDs[SlotIndex];
			SelectedFamiliarSquadIDs[OldIndex] = CharacterToSwap;

			// 예전 자리 UI 갱신 방송
			OnFamiliarSlotChanged.Broadcast(OldIndex, CharacterToSwap);
		}
	}

	//슬롯에 새 퍼밀리어 배치
	SelectedFamiliarSquadIDs[SlotIndex] = NewFamiliarID;

	UE_LOG(LogParadiseSquad, Log, TEXT("✅ [SquadSubsystem] 슬롯 %d에 퍼밀리어 %s 편성 완료"), SlotIndex, *NewFamiliarID.ToString());

	//UI에 Set 정보 발송
	OnFamiliarSlotChanged.Broadcast(SlotIndex, NewFamiliarID);
}

FName USquadSubsystem::GetFamiliarAtSlot(int32 SlotIndex) const
{
	// 슬롯이 유효하면 해당 아이디 반환, 아니면 None 반환
	if (SelectedFamiliarSquadIDs.IsValidIndex(SlotIndex))
	{
		return SelectedFamiliarSquadIDs[SlotIndex];
	}
	return NAME_None;
}

const TArray<FName>& USquadSubsystem::GetFamiliarSquad() const
{
	return SelectedFamiliarSquadIDs;
}

bool USquadSubsystem::IsFamiliarAlreadyAssigned(FName FamiliarID) const
{
	// None은 빈칸이므로 중복 취급 안 함
	if (FamiliarID.IsNone()) return false;

	// 배열 안에 해당 ID가 포함되어 있는지 검사
	return SelectedFamiliarSquadIDs.Contains(FamiliarID);
}

void USquadSubsystem::SetPlayerToSlot(int32 SlotIndex, FName NewPlayerID)
{
	//유효한 슬롯(0, 1, 2)인지 확인
	if (!SelectedPlayerSquadIDs.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogParadiseSquad, Warning, TEXT("❌ [SquadSubsystem] 유효하지 않은 슬롯 인덱스입니다: %d"), SlotIndex);
		return;
	}

	//이미 해당 슬롯에 같은 플레이어가 있다면 무시 (최적화)
	if (SelectedPlayerSquadIDs[SlotIndex] == NewPlayerID)
	{
		return;
	}

	if (!NewPlayerID.IsNone())
	{
		UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
		if (!GI) return;

		//GI 에서 데이터 유효성 검사 
		if (!GI->IsValidPlayerID(NewPlayerID))
		{
			UE_LOG(LogParadiseSquad, Error, TEXT("❌ [SquadSubsystem] 편성 실패! '%s'는 유효하지 않은 캐릭터 ID입니다."), *NewPlayerID.ToString());
			return;
		}
	}

	//중복 편성 시
	if (!NewPlayerID.IsNone() && IsPlayerAlreadyAssigned(NewPlayerID))
	{
		int32 OldIndex = SelectedPlayerSquadIDs.Find(NewPlayerID);

		if (OldIndex != INDEX_NONE)
		{
			// 현재 내가 넣으려는 타겟 슬롯에 누가 있었는지 확인 (없었다면 None일 것임)
			FName CharacterToSwap = SelectedPlayerSquadIDs[SlotIndex];

			// 타겟 슬롯에 있던 애를 내 예전 자리로 보냄
			SelectedPlayerSquadIDs[OldIndex] = CharacterToSwap;

			// 내 예전 자리가 타겟 캐릭터 얼굴로 바뀌었다고 UI에 알림
			OnPlayerSlotChanged.Broadcast(OldIndex, CharacterToSwap);
		}
	}

	//슬롯에 새 플레이어 배치
	SelectedPlayerSquadIDs[SlotIndex] = NewPlayerID;

	UE_LOG(LogParadiseSquad, Log, TEXT("✅ [SquadSubsystem] 슬롯 %d에 플레이어 %s 편성 완료"), SlotIndex, *NewPlayerID.ToString());

	//UI에 Set 정보 발송
	OnPlayerSlotChanged.Broadcast(SlotIndex, NewPlayerID);
}

FName USquadSubsystem::GetPlayerAtSlot(int32 SlotIndex) const
{
	// 슬롯이 유효하면 해당 아이디 반환, 아니면 None 반환
	if (SelectedPlayerSquadIDs.IsValidIndex(SlotIndex))
	{
		return SelectedPlayerSquadIDs[SlotIndex];
	}
	return NAME_None;
}

const TArray<FName>& USquadSubsystem::GetPlayerSquad() const
{
	return SelectedPlayerSquadIDs;
}

bool USquadSubsystem::IsPlayerAlreadyAssigned(FName PlayerID) const
{
	// None은 빈칸이므로 중복 취급 안 함
	if (PlayerID.IsNone()) return false;

	// 배열 안에 해당 ID가 포함되어 있는지 검사
	return SelectedPlayerSquadIDs.Contains(PlayerID);
}

bool USquadSubsystem::IsSquadValidForBattle(FString& OutErrorMessage) const
{
	UE_LOG(LogTemp, Warning, TEXT("================ [스쿼드 진입 검증 시작] ================"));

	// 인벤토리 서브시스템 가져오기
	UInventorySystem* InventorySys = GetGameInstance()->GetSubsystem<UInventorySystem>();

	// 1. 캐릭터 편성 검사
	bool bHasPlayer = false;
	for (int32 i = 0; i < SelectedPlayerSquadIDs.Num(); i++)
	{
		FName PlayerID = SelectedPlayerSquadIDs[i];
		UE_LOG(LogTemp, Log, TEXT("  -> Player Slot %d: [%s]"), i, *PlayerID.ToString());

		// 1️⃣ 빈 칸(None)이면 아래 로직을 무시하고 다음 슬롯으로 넘어감 (들여쓰기 방지)
		if (PlayerID.IsNone()) continue;

		bHasPlayer = true;

		// 인벤토리 시스템이나 캐릭터 데이터가 유효하지 않으면 무기 검사 건너뜀
		if (!InventorySys) continue;
		const FOwnedCharacterData* CharData = InventorySys->GetCharacterDataByID(PlayerID);
		if (!CharData) continue;

		//Find()를 사용하여 키(무기) 검색과 값 반환을 한 번에 처리 (최적화)
		const FGuid* WeaponGUID = CharData->EquipmentMap.Find(EEquipmentSlot::Weapon);

		// 무기 슬롯이 아예 없거나, 들어있는 무기 GUID가 비어있다면 거부!
		if (!WeaponGUID || !WeaponGUID->IsValid())
		{
			OutErrorMessage = TEXT("무기를 장착하지 않은 캐릭터가 있습니다.");
			UE_LOG(LogTemp, Error, TEXT("❌ 검증 실패: [%s] 캐릭터가 무기를 장착하지 않았습니다."), *PlayerID.ToString());
			return false;
		}
	}

	if (!bHasPlayer)
	{
		OutErrorMessage = TEXT("편성된 캐릭터가 없습니다.");
		UE_LOG(LogTemp, Error, TEXT("❌ 검증 실패: 편성된 캐릭터가 0명입니다."));
		return false;
	}

	// 2. 퍼밀리어 편성 검사 (5마리 필수 기준)
	int32 ValidFamiliarCount = 0;
	for (int32 i = 0; i < SelectedFamiliarSquadIDs.Num(); i++)
	{
		UE_LOG(LogTemp, Log, TEXT("  -> Familiar Slot %d: [%s]"), i, *SelectedFamiliarSquadIDs[i].ToString());
		if (!SelectedFamiliarSquadIDs[i].IsNone())
		{
			ValidFamiliarCount++;
		}
	}

	if (ValidFamiliarCount < 5)
	{
		OutErrorMessage = FString::Printf(TEXT("퍼밀리어 스쿼드를 전부 편성해주세요. (%d/5)"), ValidFamiliarCount);
		UE_LOG(LogTemp, Error, TEXT("❌ 검증 실패: 퍼밀리어 부족 (%d / 5)"), ValidFamiliarCount);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("✅ 검증 통과: 모든 스쿼드 조건 만족!"));
	return true;
}

void USquadSubsystem::LoadFromSaveGame(UParadiseSaveGame* SaveGameObj)
{
	if (!SaveGameObj) return;

	// 영웅 데이터 복구 (기존 세이브 데이터가 3칸일 때만 안전하게 가져옴)
	if (SaveGameObj->SavedPlayerSquadIDs.Num() == 3)
	{
		SelectedPlayerSquadIDs = SaveGameObj->SavedPlayerSquadIDs;
	}

	// 퍼밀리어 데이터 복구 (기존 세이브 데이터가 5칸일 때만)
	if (SaveGameObj->SavedFamiliarSquadIDs.Num() == 5)
	{
		SelectedFamiliarSquadIDs = SaveGameObj->SavedFamiliarSquadIDs;
	}

	UE_LOG(LogParadiseSquad, Log, TEXT("✅ [SquadSubsystem] 세이브 파일에서 편성 정보를 성공적으로 불러왔습니다."));
}

void USquadSubsystem::SaveToSaveGame(UParadiseSaveGame* SaveGameObj) const
{
	if (!SaveGameObj) return;

	// 서브시스템이 들고 있던 배열을 세이브 객체로 그대로 복사(전달)
	SaveGameObj->SavedPlayerSquadIDs = SelectedPlayerSquadIDs;
	SaveGameObj->SavedFamiliarSquadIDs = SelectedFamiliarSquadIDs;

	UE_LOG(LogParadiseSquad, Log, TEXT("💾 [SquadSubsystem] 세이브 객체에 편성 정보를 기록했습니다."));
}
