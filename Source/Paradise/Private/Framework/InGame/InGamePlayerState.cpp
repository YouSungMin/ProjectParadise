// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/InGame/InGamePlayerState.h"
#include "Engine/DataTable.h"
#include "Framework/System/InventorySystem.h"
#include "Components/EquipmentComponent.h"
#include "Components/CostManageComponent.h"
#include "Components/FamiliarSummonComponent.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Characters/Player/PlayerData.h"

AInGamePlayerState::AInGamePlayerState()
{
    CostManageComponent = CreateDefaultSubobject<UCostManageComponent>(TEXT("CostManageComponent"));
    FamiliarSummonComponent = CreateDefaultSubobject<UFamiliarSummonComponent>(TEXT("FamiliarSummonComponent"));
}

void AInGamePlayerState::BeginPlay()
{
    Super::BeginPlay();

	// 코스트 회복 시작
    if (CostManageComponent) CostManageComponent->StartCostRegen();
}

void AInGamePlayerState::InitSquad(const TArray<FName>& StartingHeroIDs)
{
	//인벤토리 시스템 가져오기
	UInventorySystem* InvSys = GetInventorySystem();
	if (!InvSys)
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [SquadInit] 인벤토리 시스템을 찾을 수 없습니다!"));
		return;
	}

	//배열 크기를 편성창에서 넘어온 크기로 초기화 (3으로 설정)
	SquadMembers.Init(nullptr, StartingHeroIDs.Num());

	//스쿼드 영웅 최대치 크기(3)만큼 for문
	for (int32 i = 0; i < StartingHeroIDs.Num(); ++i)
	{
		FName HeroID = StartingHeroIDs[i];

		// 빈 슬롯이면 스폰하지 않고 넘어가기
		if (HeroID.IsNone()) continue;

		UClass* SpawnClass = APlayerData::StaticClass();

		if (PlayerDataClass)
		{
			SpawnClass = PlayerDataClass;
		}
		APlayerData* NewSoul = GetWorld()->SpawnActor<APlayerData>(SpawnClass);

		if (NewSoul)
		{
			//장비 컴포넌트 초기화
			if (UEquipmentComponent* EquipComp = NewSoul->GetEquipmentComponent())
			{
				//데이터 검색
				if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(HeroID))
				{
					// 찾은 데이터의 장비 맵으로 초기화
					EquipComp->InitializeEquipment(CharData->EquipmentMap);
					//UE_LOG(LogTemp, Log, TEXT("🔗 [SquadInit] %s 장비 데이터 동기화 완료"), *HeroID.ToString());
				}
				/*else
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠️ [SquadInit] 인벤토리에 %s 데이터가 없습니다."), *HeroID.ToString());
				}*/
			}
			//캐릭터 데이터 초기화
			NewSoul->InitPlayerData(HeroID);

			//정확한 슬롯 위치[i]에 저장합니다.
			SquadMembers[i] = NewSoul;
		}
	}

	//UE_LOG(LogTemp, Log, TEXT("✅ [PlayerState] 스쿼드 초기화 완료 (크기: %d)"), SquadMembers.Num());
}

UInventorySystem* AInGamePlayerState::GetInventorySystem() const
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



APlayerData* AInGamePlayerState::GetSquadMemberData(int32 Index) const
{
    if (SquadMembers.IsValidIndex(Index))
    {
        return SquadMembers[Index];
    }
    return nullptr;
}
