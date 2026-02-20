// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/SquadSubsystem.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"
#include "Framework/Core/ParadiseGameInstance.h"

void USquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 3인 플레이어 스쿼드 배열을 NAME_None으로 초기화 (크기 3 고정)
	SelectedPlayerSquadIDs.Init(NAME_None, 3);

	// TODO: 게임 시작 시 저장된 스쿼드 정보가 있다면 여기서 불러옵니다.
	// LoadSquadData();
}

void USquadSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void USquadSubsystem::SetPlayerToSlot(int32 SlotIndex, FName NewPlayerID)
{
	//유효한 슬롯(0, 1, 2)인지 확인
	if (!SelectedPlayerSquadIDs.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ [SquadSubsystem] 유효하지 않은 슬롯 인덱스입니다: %d"), SlotIndex);
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
			UE_LOG(LogTemp, Error, TEXT("❌ [SquadSubsystem] 편성 실패! '%s'는 유효하지 않은 캐릭터 ID입니다."), *NewPlayerID.ToString());
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

	UE_LOG(LogTemp, Log, TEXT("✅ [SquadSubsystem] 슬롯 %d에 플레이어 %s 편성 완료"), SlotIndex, *NewPlayerID.ToString());

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

void USquadSubsystem::SaveSquadData()
{
	
}
