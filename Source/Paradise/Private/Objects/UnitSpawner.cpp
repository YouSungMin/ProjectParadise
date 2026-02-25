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

	// 1. GameInstance를 통해 테이블 데이터 자동 로드
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (GI && GI->StageWaveDetailDataTable && !TargetStageID.IsNone())
	{
		WaveConfigs.Empty();

		TArray<FStageWaveDetail*> AllRows;
		// GI_Paradise에 할당된 테이블에서 모든 행을 가져옴
		GI->StageWaveDetailDataTable->GetAllRows<FStageWaveDetail>(TEXT(""), AllRows);

		for (FStageWaveDetail* Row : AllRows)
		{
			// TargetStageID가 일치하는 웨이브만 필터링
			if (Row && Row->TargetStageID == TargetStageID)
			{
				FWaveConfig NewConfig;
				NewConfig.UnitRowName = Row->MonsterID;      // MonsterID 매핑
				NewConfig.SpawnCount = Row->SpawnCount;      // SpawnCount 매핑
				NewConfig.SpawnInterval = Row->SpawnInterval;// SpawnInterval 매핑
				NewConfig.NextWaveDelay = Row->PreWaveDelay; // PreWaveDelay를 딜레이로 사용

				WaveConfigs.Add(NewConfig);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("✅ [Spawner] 스테이지 '%s' 데이터 로드 완료 (%d 웨이브)"), *TargetStageID.ToString(), WaveConfigs.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [Spawner] 데이터 로드 실패! GI 또는 Table이 유효하지 않거나 TargetStageID가 없습니다."));
	}

	// 2. 오브젝트 풀 초기화
	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	if (PoolSubsystem && UnitClass)
	{
		for (int32 i = 0; i < PreSpawnCount; i++)
		{
			AUnitBase* TempUnit = PoolSubsystem->SpawnPoolActor<AUnitBase>(UnitClass, GetActorLocation(), GetActorRotation(), this, nullptr);
			if (TempUnit) PoolSubsystem->ReturnToPool(TempUnit);
		}
	}

	// 3. 첫 웨이브 시작
	if (WaveConfigs.Num() > 0)
	{
		// 첫 소환은 PreWaveDelay만큼 대기 후 시작하도록 타이머 설정
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AUnitSpawner::SpawnUnit,
			WaveConfigs[0].SpawnInterval, true, WaveConfigs[0].NextWaveDelay);
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

		// 유닛 데이터 로드 (GI 참조)
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
								}
							}
						}
					}
				}
			}
		}
	}

	// 웨이브 진행 체크
	CurrentSpawnCountInWave++;
	if (CurrentSpawnCountInWave >= WaveConfigs[CurrentWaveIndex].SpawnCount)
	{
		CurrentSpawnCountInWave = 0;
		int32 FinishedIdx = CurrentWaveIndex;
		CurrentWaveIndex++;

		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

		if (WaveConfigs.IsValidIndex(CurrentWaveIndex))
		{
			// 다음 웨이브로 전환 시 간격과 딜레이 적용
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