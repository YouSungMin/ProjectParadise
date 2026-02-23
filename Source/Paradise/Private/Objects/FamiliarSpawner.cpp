// Fill out your copyright notice in the Description page of Project Settings.

#include "Objects/FamiliarSpawner.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Framework/InGame/MyAIController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Kismet/GameplayStatics.h" 
#include "Data/Structs/UnitStructs.h"
#include "Objects/HomeBase.h"

void AFamiliarSpawner::SpawnFamiliarByID(FName UnitID)
{
	if (UnitID.IsNone()) return;

	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());

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

							UBlackboardComponent* BB = AIC->GetBlackboardComponent();
							if (BB)
							{
								TArray<AActor*> FoundBases;

								UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHomeBase::StaticClass(), FoundBases);

								AHomeBase* TargetEnemyBase = nullptr;

								for (AActor* BaseActor : FoundBases)
								{
									AHomeBase* HomeBase = Cast<AHomeBase>(BaseActor);
									if (!HomeBase) continue;

									if (NewUnit->IsHostile(HomeBase))
									{
										TargetEnemyBase = HomeBase;
										break;
									}
								}

								if (TargetEnemyBase)
								{
									// 블랙보드 오브젝트 세팅
									BB->SetValueAsObject(FName("EnemyBaseActor"), TargetEnemyBase);

									FVector BaseLoc = TargetEnemyBase->GetActorLocation();
									FVector UnitLoc = NewUnit->GetActorLocation();
									FVector Dir = (UnitLoc - BaseLoc).GetSafeNormal();

									// 기지 중심에서 약 200유닛 떨어진 지점을 최종 목표로 설정 (캡슐 충돌 회피)
									FVector TargetLocation = BaseLoc + (Dir * 200.0f);
									BB->SetValueAsVector(FName("MoveLocation"), TargetLocation);

									UE_LOG(LogTemp, Log, TEXT("Familiar [%s] -> 적 기지 [%s] 추적 시작"), *UnitID.ToString(), *TargetEnemyBase->GetName());
								}
								else
								{
									UE_LOG(LogTemp, Error, TEXT("적대적인 기지를 찾지 못했습니다. 기지의 FactionTag를 확인하세요."));
								}
							}

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
	}
}