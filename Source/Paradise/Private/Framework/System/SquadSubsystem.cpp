// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/SquadSubsystem.h"

void USquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SelectedHeroSquadIDs.Init(NAME_None, 3);
}

void USquadSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void USquadSubsystem::SetHeroToSlot(int32 SlotIndex, FName NewHeroID)
{
    // 유효한 슬롯(0, 1, 2)인지 확인
    if (SelectedHeroSquadIDs.IsValidIndex(SlotIndex))
    {
        // 중복 체크 로직 (이미 다른 슬롯에 같은 영웅이 있다면 무시하거나 스왑 처리)
        if (IsHeroAlreadyAssigned(NewHeroID)) return;

        SelectedHeroSquadIDs[SlotIndex] = NewHeroID;

        // UI에 알림 
        OnHeroSlotChanged.Broadcast(SlotIndex, NewHeroID);
    }
}

FName USquadSubsystem::GetHeroAtSlot(int32 SlotIndex) const
{
    return SelectedHeroSquadIDs[SlotIndex];
}

const TArray<FName>& USquadSubsystem::GetHeroSquad() const
{
    return SelectedHeroSquadIDs;
}

bool USquadSubsystem::IsHeroAlreadyAssigned(FName HeroID) const
{
    if (HeroID.IsNone()) return false;
    return SelectedHeroSquadIDs.Contains(HeroID);
}

void USquadSubsystem::SaveSquadData()
{
}
