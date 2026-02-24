// Fill out your copyright notice in the Description page of Project Settings.

#include "Objects/FamiliarSpawner.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Framework/InGame/MyAIController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BrainComponent.h"
#include "Data/Structs/UnitStructs.h"

void AFamiliarSpawner::SpawnFamiliarByID(FName UnitID)
{
	if (UnitID.IsNone()) return;

	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());

	// GI가 존재하는지 확인
	if (!PoolSubsystem || !UnitClass || !GI)
	{
		UE_LOG(LogTemp, Error, TEXT("FamiliarSpawner: 시스템 또는 GI 로드 실패"));
		return;
	}

	FVector SpawnLocation = GetRandomSpawnLocation() + FVector(0.f, 0.f, 100.0f);
	FRotator SpawnRotation = GetActorRotation();

	AUnitBase* NewUnit = PoolSubsystem->SpawnPoolActor<AUnitBase>(UnitClass, SpawnLocation, SpawnRotation, this, nullptr);

	if (NewUnit)
	{
		NewUnit->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::ResetPhysics);
		NewUnit->SetUnitID(UnitID);

		// GI에 등록된 Familiar 전용 테이블 참조
		if (GI->FamiliarStatsDataTable && GI->FamiliarAssetsDataTable)
		{
			FFamiliarStats* StatData = GI->GetDataTableRow<FFamiliarStats>(GI->FamiliarStatsDataTable, UnitID);
			FFamiliarAssets* AssetData = GI->GetDataTableRow<FFamiliarAssets>(GI->FamiliarAssetsDataTable, UnitID);

			if (StatData && AssetData)
			{
				NewUnit->InitializeUnit(StatData, AssetData);

				AMyAIController* AIC = Cast<AMyAIController>(NewUnit->GetController());
				if (!AIC)
				{
					NewUnit->SpawnDefaultController();
					AIC = Cast<AMyAIController>(NewUnit->GetController());
				}

				if (AIC)
				{
					if (AIC->GetPawn() != NewUnit)
					{
						AIC->Possess(NewUnit);
					}

					if (!AssetData->BehaviorTree.IsNull())
					{
						UBehaviorTree* BT = AssetData->BehaviorTree.LoadSynchronous();
						if (BT)
						{
							AIC->RunBehaviorTree(BT);
							if (AIC->GetBrainComponent()) AIC->GetBrainComponent()->RestartLogic();
						}
					}
				}
				UE_LOG(LogTemp, Warning, TEXT("Successfully Spawned Familiar via GI: %s"), *UnitID.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Familiar Table에서 ID [%s]를 찾을 수 없습니다."), *UnitID.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GI에 Familiar 데이터 테이블이 등록되지 않았습니다."));
		}
	}
}