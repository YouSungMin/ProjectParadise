// Fill out your copyright notice in the Description page of Project Settings.

#include "Objects/UnitSpawner.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Framework/InGame/MyAIController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Objects/HomeBase.h"

AUnitSpawner::AUnitSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AUnitSpawner::BeginPlay()
{
	Super::BeginPlay();

	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	if (PoolSubsystem && UnitClass)
	{
		for (int32 i = 0; i < PreSpawnCount; i++)
		{
			AUnitBase* TempUnit = PoolSubsystem->SpawnPoolActor<AUnitBase>(UnitClass, GetActorLocation(), GetActorRotation(), this, nullptr);
			if (TempUnit) PoolSubsystem->ReturnToPool(TempUnit);
		}
	}

	if (WaveConfigs.Num() > 0)
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AUnitSpawner::SpawnUnit, WaveConfigs[0].SpawnInterval, true, 1.0f);
	}
}

void AUnitSpawner::SpawnUnit()
{
	if (!WaveConfigs.IsValidIndex(CurrentWaveIndex))
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	EnemyRowName = WaveConfigs[CurrentWaveIndex].UnitRowName;
	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());

	if (!PoolSubsystem || !UnitClass || EnemyRowName.IsNone() || !GI) return;

	FVector SpawnLocation = GetRandomSpawnLocation() + FVector(0.f, 0.f, 100.0f);
	FRotator SpawnRotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);

	AUnitBase* NewUnit = PoolSubsystem->SpawnPoolActor<AUnitBase>(UnitClass, SpawnLocation, SpawnRotation, this, nullptr);

	if (NewUnit)
	{
		NewUnit->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::ResetPhysics);
		NewUnit->SetUnitID(EnemyRowName);

		if (GI->EnemyStatsDataTable && GI->EnemyAssetsDataTable)
		{
			FEnemyStats* StatData = GI->GetDataTableRow<FEnemyStats>(GI->EnemyStatsDataTable, NewUnit->GetUnitID());
			FEnemyAssets* AssetData = GI->GetDataTableRow<FEnemyAssets>(GI->EnemyAssetsDataTable, NewUnit->GetUnitID());

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
					AIC->Possess(NewUnit);

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

								AHomeBase* TargetFriendlyBase = nullptr;

								for (AActor* BaseActor : FoundBases)
								{
									AHomeBase* HomeBase = Cast<AHomeBase>(BaseActor);
									if (HomeBase && NewUnit->IsHostile(HomeBase))
									{
										TargetFriendlyBase = HomeBase;
										break;
									}
								}

								if (TargetFriendlyBase)
								{
									BB->SetValueAsObject(FName("EnemyBaseActor"), TargetFriendlyBase);

									FVector BaseLoc = TargetFriendlyBase->GetActorLocation();
									FVector Dir = (NewUnit->GetActorLocation() - BaseLoc).GetSafeNormal();
									FVector TargetLocation = BaseLoc + (Dir * 200.0f);

									BB->SetValueAsVector(FName("MoveLocation"), TargetLocation);
									BB->SetValueAsObject(FName("HomeBaseActor"), TargetFriendlyBase);

									UE_LOG(LogTemp, Log, TEXT("[%s] BT Started with Target: %s"), *NewUnit->GetName(), *TargetFriendlyBase->GetName());
								}
								else
								{
									if (APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn())
									{
										BB->SetValueAsVector(FName("MoveLocation"), PlayerPawn->GetActorLocation());
										UE_LOG(LogTemp, Log, TEXT("[%s] BT Started with Target: Player"), *NewUnit->GetName());
									}
								}
							}
						}
					}
				}
			}
		}
	}

	CurrentSpawnCountInWave++;
	if (CurrentSpawnCountInWave >= WaveConfigs[CurrentWaveIndex].SpawnCount)
	{
		CurrentSpawnCountInWave = 0;
		int32 FinishedIdx = CurrentWaveIndex;
		CurrentWaveIndex++;

		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

		if (WaveConfigs.IsValidIndex(CurrentWaveIndex))
		{
			GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AUnitSpawner::SpawnUnit,
				WaveConfigs[CurrentWaveIndex].SpawnInterval, true, WaveConfigs[FinishedIdx].NextWaveDelay);
		}
	}
}

FVector AUnitSpawner::GetRandomSpawnLocation()
{
	FVector Origin = GetActorLocation();
	FVector TargetPoint = Origin + FVector(FMath::RandRange(-SpawnExtent.X, SpawnExtent.X), FMath::RandRange(-SpawnExtent.Y, SpawnExtent.Y), 0.0f);

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation NavLocation;
	if (NavSys && NavSys->ProjectPointToNavigation(TargetPoint, NavLocation, FVector(0.f, 0.f, 500.f)))
	{
		return NavLocation.Location;
	}
	return TargetPoint;
}

#if WITH_EDITOR
void AUnitSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	DrawDebugBox(GetWorld(), GetActorLocation(), SpawnExtent, FColor::Cyan, false, 2.0f, 0, 5.0f);
}
#endif