// Fill out your copyright notice in the Description page of Project Settings.

#include "Objects/UnitSpawner.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Characters/AIUnit/SkillCasterUnit.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Framework/InGame/MyAIController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/StageSubsystem.h"
#include "Components/CapsuleComponent.h"
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

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	// 1. StageSubsystem에서 현재 선택된 StageID 자동으로 가져오기
	if (UStageSubsystem* StageSys = GI->GetSubsystem<UStageSubsystem>())
	{
		TargetStageID = StageSys->GetSelectedStageID();

		//UE_LOG(LogTemp, Log, TEXT("🚀 [Spawner] 선택된 스테이지 '%s'를 서브시스템에서 로드했습니다."), *TargetStageID.ToString());
	}

	// 2. 로드된 ID를 바탕으로 웨이브 테이블 구성
	if (GI->StageWaveDetailDataTable && !TargetStageID.IsNone())
	{
		WaveConfigs.Empty();
		TArray<FStageWaveDetail*> AllRows;
		GI->StageWaveDetailDataTable->GetAllRows<FStageWaveDetail>(TEXT(""), AllRows);

		for (FStageWaveDetail* Row : AllRows)
		{
			if (Row && Row->TargetStageID == TargetStageID)
			{
				FWaveConfig NewConfig;
				NewConfig.UnitRowName = Row->MonsterID;
				NewConfig.SpawnCount = Row->SpawnCount;
				NewConfig.SpawnInterval = Row->SpawnInterval;
				NewConfig.NextWaveDelay = Row->PreWaveDelay;
				WaveConfigs.Add(NewConfig);
			}
		}
	}

	// 3. 오브젝트 풀링 및 타이머 시작
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
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AUnitSpawner::SpawnUnit,
			WaveConfigs[0].SpawnInterval, true, WaveConfigs[0].NextWaveDelay);
	}
}

//void AUnitSpawner::SpawnUnit()
//{
//	if (!WaveConfigs.IsValidIndex(CurrentWaveIndex))
//	{
//		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
//		return;
//	}
//
//	EnemyRowName = WaveConfigs[CurrentWaveIndex].UnitRowName;
//	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
//	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
//
//	if (!PoolSubsystem || (!UnitClass && !SkillCasterClass) || EnemyRowName.IsNone() || !GI) return;
//
//	FEnemyStats* StatData = nullptr;
//	FEnemyAssets* AssetData = nullptr;
//
//	if (GI->EnemyStatsDataTable && GI->EnemyAssetsDataTable)
//	{
//		StatData = GI->GetDataTableRow<FEnemyStats>(GI->EnemyStatsDataTable, EnemyRowName);
//		AssetData = GI->GetDataTableRow<FEnemyAssets>(GI->EnemyAssetsDataTable, EnemyRowName);
//	}
//
//	if (!StatData || !AssetData) return;
//
//	TSubclassOf<AUnitBase> ClassToSpawn = UnitClass;
//
//	if (StatData->PatternActionHandles.Num() > 0 && SkillCasterClass)
//	{
//		ClassToSpawn = SkillCasterClass; // 보스(스킬 캐스터) 클래스로 변경!
//	}
//
//	// 유배지에서 복귀할 정확한 전장 좌표 계산
//	FVector SpawnLocation = GetRandomSpawnLocation() + FVector(0.f, 0.f, 20.0f);
//	FRotator SpawnRotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);
//
//	// 풀에서 유닛을 가져옴 (이때 내부적으로 OnPoolActivate 호출)
//	AUnitBase* NewUnit = PoolSubsystem->SpawnPoolActor<AUnitBase>(ClassToSpawn, SpawnLocation, SpawnRotation, this, nullptr);
//
//	if (NewUnit)
//	{
//		// 전장 좌표로 즉시 순간이동 및 물리 리셋
//		NewUnit->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::ResetPhysics);
//
//		NewUnit->SetUnitID(EnemyRowName);
//		// 스탯 및 메시 초기화
//		NewUnit->InitializeUnit(StatData, AssetData);
//
//		if (AssetData->AIController)
//		{
//			NewUnit->AIControllerClass = AssetData->AIController;
//		}
//
//		// 컨트롤러가 없으면 스폰합니다. (이제 테이블에 설정한 보스 컨트롤러가 스폰됨)
//		if (!NewUnit->GetController())
//		{
//			NewUnit->SpawnDefaultController();
//		}
//
//		AAIController* AIC = Cast<AAIController>(NewUnit->GetController());
//		
//
//		if (AIC)
//		{
//			AIC->Possess(NewUnit);
//
//			if (!AssetData->BehaviorTree.IsNull())
//			{
//				UBehaviorTree* BT = AssetData->BehaviorTree.LoadSynchronous();
//				if (BT)
//				{
//					// BT 실행
//					AIC->RunBehaviorTree(BT);
//
//					// 🚨 [수정 3] 보스도 쉴 때 본진으로 걸어가야 하므로 똑같이 타겟팅 해줍니다.
//					UBlackboardComponent* BB = AIC->GetBlackboardComponent();
//					if (BB)
//					{
//						TArray<AActor*> FoundBases;
//						UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHomeBase::StaticClass(), FoundBases);
//
//						AHomeBase* TargetFriendlyBase = nullptr;
//						for (AActor* BaseActor : FoundBases)
//						{
//							AHomeBase* HomeBase = Cast<AHomeBase>(BaseActor);
//							if (HomeBase && NewUnit->IsHostile(HomeBase)) // 기존에 작성하신 IsHostile 유지
//							{
//								TargetFriendlyBase = HomeBase;
//								break;
//							}
//						}
//
//						if (TargetFriendlyBase)
//						{
//							// BB에 목적지(본진) 세팅
//							BB->SetValueAsObject(FName("EnemyBaseActor"), TargetFriendlyBase);
//							BB->SetValueAsObject(FName("HomeBaseActor"), TargetFriendlyBase);
//
//							FVector BaseLoc = TargetFriendlyBase->GetActorLocation();
//							FVector Dir = (NewUnit->GetActorLocation() - BaseLoc).GetSafeNormal();
//							FVector TargetLocation = BaseLoc + (Dir * 200.0f);
//
//							BB->SetValueAsVector(FName("MoveLocation"), TargetLocation);
//						}
//					}
//					// AI 로직 강제 재시작
//					if (AIC->GetBrainComponent()) AIC->GetBrainComponent()->RestartLogic();
//				}
//			}
//		}
//	}
//
//	// 웨이브 진행 관리
//	CurrentSpawnCountInWave++;
//	if (CurrentSpawnCountInWave >= WaveConfigs[CurrentWaveIndex].SpawnCount)
//	{
//		CurrentSpawnCountInWave = 0;
//		int32 FinishedIdx = CurrentWaveIndex;
//		CurrentWaveIndex++;
//		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
//		if (WaveConfigs.IsValidIndex(CurrentWaveIndex))
//		{
//			GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AUnitSpawner::SpawnUnit,
//				WaveConfigs[CurrentWaveIndex].SpawnInterval, true, WaveConfigs[FinishedIdx].NextWaveDelay);
//		}
//	}
//}
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

	if (!PoolSubsystem || (!UnitClass && !SkillCasterClass) || EnemyRowName.IsNone() || !GI) return;

	FEnemyStats* StatData = nullptr;
	FEnemyAssets* AssetData = nullptr;

	if (GI->EnemyStatsDataTable && GI->EnemyAssetsDataTable)
	{
		StatData = GI->GetDataTableRow<FEnemyStats>(GI->EnemyStatsDataTable, EnemyRowName);
		AssetData = GI->GetDataTableRow<FEnemyAssets>(GI->EnemyAssetsDataTable, EnemyRowName);
	}

	if (!StatData || !AssetData) return;

	TSubclassOf<AUnitBase> ClassToSpawn = UnitClass;

	if (StatData->PatternActionHandles.Num() > 0 && SkillCasterClass)
	{
		ClassToSpawn = SkillCasterClass; // 보스(스킬 캐스터) 클래스로 변경!
	}

	// 유배지에서 복귀할 정확한 전장 좌표 계산
	FVector SpawnLocation = GetRandomSpawnLocation() + FVector(0.f, 0.f, 20.0f);
	FRotator SpawnRotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);

	// 풀에서 유닛을 가져옴 (이때 내부적으로 OnPoolActivate 호출)
	AUnitBase* NewUnit = PoolSubsystem->SpawnPoolActor<AUnitBase>(ClassToSpawn, SpawnLocation, SpawnRotation, this, nullptr);

	if (NewUnit)
	{
		// 1. 전장 좌표로 즉시 순간이동 및 물리 리셋
		NewUnit->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::ResetPhysics);

		// 2. 스탯 및 데이터 초기화
		NewUnit->SetUnitID(EnemyRowName);
		NewUnit->InitializeUnit(StatData, AssetData);

		if (UCapsuleComponent* CapsuleComp = NewUnit->GetCapsuleComponent())
		{
			CapsuleComp->SetCollisionProfileName(TEXT("EnemyPreset"));
		}

		// 3. 데이터 테이블에 설정된 전용 AI 컨트롤러 클래스 장착
		if (AssetData->AIController)
		{
			NewUnit->AIControllerClass = AssetData->AIController;
		}

		// ==========================================================
		// 🚨 4. AI 컨트롤러 강제 교체 및 빙의 (수정됨)
		// ==========================================================
		AController* CurrentCtrl = NewUnit->GetController();

		// 현재 컨트롤러가 존재하지만, 데이터테이블에 세팅된 클래스와 다르다면 (예: 풀링된 이전 유닛의 뇌) 파괴합니다.
		if (CurrentCtrl && CurrentCtrl->GetClass() != NewUnit->AIControllerClass)
		{
			CurrentCtrl->UnPossess();
			CurrentCtrl->Destroy();
		}

		// 컨트롤러가 없거나 방금 파괴되었다면, AIControllerClass를 기반으로 새로 생성합니다.
		if (!NewUnit->GetController())
		{
			NewUnit->SpawnDefaultController();
		}

		AAIController* AIC = Cast<AAIController>(NewUnit->GetController());

		if (AIC)
		{
			// 연결이 안 되어 있다면 강제 빙의
			if (AIC->GetPawn() != NewUnit)
			{
				AIC->Possess(NewUnit);
			}
			else
			{
				// 이미 정상적으로 빙의된 상태라면 뇌(Brain)만 재시작
				if (AIC->GetBrainComponent())
				{
					AIC->GetBrainComponent()->RestartLogic();
				}
			}

			// 레인(Lane) Y축 값 주입
			UBlackboardComponent* BB = AIC->GetBlackboardComponent();
			if (BB)
			{
				float LanePositions[] = { 400.0f, 100.0f, -300.0f };
				int32 RandomLaneIndex = FMath::RandRange(0, 2);
				BB->SetValueAsFloat(FName("AssignedLaneY"), LanePositions[RandomLaneIndex]);

				float MyAttackRange = 100.0f; // 기본값 (안전 장치)
				if (NewUnit)
				{
					// UnitBase에 구현된 사거리 반환 함수를 호출하여 가져옵니다.
					// (만약 이름이 다르다면 프로젝트에 맞게 GetAttackRange() 부분을 수정해 주세요)
					MyAttackRange = NewUnit->GetAttackRange();
				}
				BB->SetValueAsFloat(FName("AttackRange"), MyAttackRange);
			}
		}
	}

	// 5. 웨이브 진행 관리 (기존과 동일)
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