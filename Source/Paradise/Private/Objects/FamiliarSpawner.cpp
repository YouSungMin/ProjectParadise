// Fill out your copyright notice in the Description page of Project Settings.

#include "Objects/FamiliarSpawner.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Framework/InGame/MyAIController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "BrainComponent.h"
#include "Kismet/GameplayStatics.h" 
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "Objects/HomeBase.h"

AFamiliarSpawner::AFamiliarSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFamiliarSpawner::BeginPlay()
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
	//UE_LOG(LogTemp, Log, TEXT("🛡️ FamiliarSpawner: 초기화 및 오브젝트 풀링 완료."));
}

void AFamiliarSpawner::SpawnFamiliarByID(FName UnitID)
{
	if (UnitID.IsNone()) return;

	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());

	if (!PoolSubsystem || !UnitClass || !GI) return;

	// 전장 스폰 위치 계산
	FVector SpawnLocation = GetRandomSpawnLocation() + FVector(0.f, 0.f, 20.0f);
	FRotator SpawnRotation = GetActorRotation();

	// 풀에서 유닛 확보
	AUnitBase* NewUnit = PoolSubsystem->SpawnPoolActor<AUnitBase>(UnitClass, SpawnLocation, SpawnRotation, this, nullptr);

	if (NewUnit)
	{
		// 전장으로 강제 워프 및 물리 상태 초기화
		NewUnit->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::ResetPhysics);

		NewUnit->SetUnitID(UnitID);

		if (GI->FamiliarStatsDataTable && GI->FamiliarAssetsDataTable)
		{
			FFamiliarStats* StatData = GI->GetDataTableRow<FFamiliarStats>(GI->FamiliarStatsDataTable, UnitID);
			FFamiliarAssets* AssetData = GI->GetDataTableRow<FFamiliarAssets>(GI->FamiliarAssetsDataTable, UnitID);

			if (StatData && AssetData)
			{
				NewUnit->InitializeUnit(StatData, AssetData);

				// 데이터 테이블의 컨트롤러 클래스 세팅
				if (AssetData->AIController)
				{
					NewUnit->AIControllerClass = AssetData->AIController;
				}

				if (UCapsuleComponent* CapsuleComp = NewUnit->GetCapsuleComponent())
				{
					CapsuleComp->SetCollisionProfileName(TEXT("AllyPreset"));
				}

				// ==========================================================
				// 🚨 AI 컨트롤러 강제 교체 (수정됨)
				// ==========================================================
				AController* CurrentCtrl = NewUnit->GetController();

				if (CurrentCtrl && CurrentCtrl->GetClass() != NewUnit->AIControllerClass)
				{
					CurrentCtrl->UnPossess();
					CurrentCtrl->Destroy();
				}

				if (!NewUnit->GetController())
				{
					NewUnit->SpawnDefaultController();
				}

				AAIController* AIC = Cast<AAIController>(NewUnit->GetController());

				if (AIC)
				{
					// 안전한 빙의 처리
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
								// 레인 배정
								float LanePositions[] = { 400.0f, 100.0f, -300.0f };
								int32 RandomLaneIndex = FMath::RandRange(0, 2);
								BB->SetValueAsFloat(FName("AssignedLaneY"), LanePositions[RandomLaneIndex]);

								float MyAttackRange = 100.0f; // 기본값 (안전 장치)
								if (NewUnit)
								{
									MyAttackRange = NewUnit->GetAttackRange();
								}
								BB->SetValueAsFloat(FName("AttackRange"), MyAttackRange);

								// 기지 타겟팅
								TArray<AActor*> FoundBases;
								UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHomeBase::StaticClass(), FoundBases);

								AHomeBase* TargetEnemyBase = nullptr;
								for (AActor* BaseActor : FoundBases)
								{
									AHomeBase* HomeBase = Cast<AHomeBase>(BaseActor);
									if (HomeBase && NewUnit->IsHostile(HomeBase))
									{
										TargetEnemyBase = HomeBase;
										break;
									}
								}

								if (TargetEnemyBase)
								{
									BB->SetValueAsObject(FName("EnemyBaseActor"), TargetEnemyBase);
									FVector BaseLoc = TargetEnemyBase->GetActorLocation();
									FVector Dir = (NewUnit->GetActorLocation() - BaseLoc).GetSafeNormal();
									FVector TargetLocation = BaseLoc + (Dir * 200.0f);
									BB->SetValueAsVector(FName("MoveLocation"), TargetLocation);
								}
							}
							if (AIC->GetBrainComponent()) AIC->GetBrainComponent()->RestartLogic();
						}
					}
				}
				//UE_LOG(LogTemp, Log, TEXT("✅ Familiar Spawned: %s"), *UnitID.ToString());
			}
		}
	}
}

FVector AFamiliarSpawner::GetRandomSpawnLocation()
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
void AFamiliarSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	DrawDebugBox(GetWorld(), GetActorLocation(), SpawnExtent, FColor::Green, false, 2.0f, 0, 5.0f);
}
#endif