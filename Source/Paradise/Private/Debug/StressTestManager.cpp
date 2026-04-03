// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/StressTestManager.h"
#include "Characters/Base/PlayerBase.h"
#include "Components/EquipmentComponent.h" // 장비 컴포넌트 헤더
#include "Characters/Player/PlayerData.h" // 영혼 헤더

AStressTestManager::AStressTestManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AStressTestManager::BeginPlay()
{
	Super::BeginPlay();

	if (!TestUnitClass) return;

	// 1. 대량 스폰 테스트 (InitSquad 부하 체크)
	for (int32 i = 0; i < SpawnCount; i++)
	{
		FVector SpawnLoc = GetActorLocation() + FVector(FMath::RandRange(-1000, 1000), FMath::RandRange(-1000, 1000), 0);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APlayerBase* NewUnit = GetWorld()->SpawnActor<APlayerBase>(TestUnitClass, SpawnLoc, FRotator::ZeroRotator, Params);
		if (NewUnit)
		{
			SpawnedUnits.Add(NewUnit);

			// ※ 주의: 여기서 PlayerData(영혼)도 같이미리 만들어줘야 제대로 동작합니다.
			// 실제로는 InGamePlayerState가 하는 일을 약식으로 구현
			APlayerData* MockSoul = GetWorld()->SpawnActor<APlayerData>();
			NewUnit->InitializePlayer(MockSoul); // 최적화된 초기화 함수 호출
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("🔥 [Stress] %d 유닛 스폰 완료!"), SpawnedUnits.Num());
}

void AStressTestManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 2. 무기 교체 고문 테스트 (UpdateVisuals 부하 체크)
	if (bTortureMode)
	{
		static bool bToggle = false;
		bToggle = !bToggle;

		FName WeaponID = bToggle ? FName("Iron_Sword") : FName("Wooden_Staff"); // 실제 존재하는 아이템 ID 사용

		for (APlayerBase* Unit : SpawnedUnits)
		{
			if (Unit && Unit->GetPlayerData())
			{
				// 매 프레임 모든 유닛의 장비를 강제로 변경
				// 우리가 최적화한 SetSkeletalMesh 방식이 빛을 발하는 순간입니다.
				if (UEquipmentComponent* Equip = Unit->GetPlayerData()->GetEquipmentComponent())
				{
					// 테스트를 위해 강제 호출 (실제로는 데이터 변경 후 호출해야 함)
					// 여기서는 EquipComp 내부에 테스트용 함수를 하나 더 파거나, 
					//Equip->TestEquip(Unit, EEquipmentSlot::Weapon, WeaponID);
				}
			}
		}
	}
}