// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Unit/BTService_LaneTargeting.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Objects/HomeBase.h" 
#include "Characters/AIUnit/UnitBase.h" // AUnitBase 및 IsEnemy 함수 사용을 위해 포함
#include "Characters/Base/CharacterBase.h"
#include "Engine/Engine.h" // 🚨 로그 출력을 위해 추가

UBTService_LaneTargeting::UBTService_LaneTargeting()
{
	NodeName = TEXT("Lane Targeting");
	bNotifyTick = true;
	Interval = 0.5f;
	RandomDeviation = 0.1f;

	TargetObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
}

void UBTService_LaneTargeting::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!BBComp || !AICon || !AICon->GetPawn()) return;

	APawn* ControlledPawn = AICon->GetPawn();
	AUnitBase* MyUnit = Cast<AUnitBase>(ControlledPawn); // 내 유닛 캐싱
	FVector MyLocation = ControlledPawn->GetActorLocation();

	if (!MyUnit) return;

	AActor* ExistingTarget = Cast<AActor>(BBComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	AUnitBase* ExistingEnemy = Cast<AUnitBase>(ExistingTarget); // 기지가 아닌 실제 적 유닛인지 확인

	if (ExistingEnemy && !ExistingEnemy->IsDead())
	{
		// 적이 아직 내 탐색 반경(SearchRadius) 안에 있다면 한눈팔지 않음!
		if (MyUnit->GetDistanceTo(ExistingEnemy) <= SearchRadius)
		{
			// 🚨 로그: 타겟 유지 중 (록온)
			// 너무 많이 뜨면 화면을 가릴 수 있으므로, 필요시 주석 처리하세요.
			// if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Cyan, FString::Printf(TEXT("[%s] 🔒 타겟 록온 유지: %s"), *MyUnit->GetName(), *ExistingEnemy->GetName()));
			return; // 아래의 SphereOverlap 색적 로직을 아예 돌리지 않고 종료
		}
	}

	// 1. 반경 내 액터 탐색 (Sphere Overlap)
	TArray<AActor*> OutActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(ControlledPawn);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		MyLocation,
		SearchRadius,
		TargetObjectTypes,
		nullptr,
		ActorsToIgnore,
		OutActors
	);

	AActor* ClosestEnemy = nullptr;
	float MinDistanceSq = MAX_flt;

	// 2. 가장 가까운 '적군'이면서 '살아있는' 액터 찾기
	for (AActor* OverlappedActor : OutActors)
	{
		AUnitBase* TargetUnit = Cast<AUnitBase>(OverlappedActor);

		// 유닛이 아니거나, 죽었거나, 내 편(IsEnemy가 false)이라면 무시
		if (!TargetUnit || TargetUnit->IsDead() || !MyUnit->IsEnemy(TargetUnit))
		{
			continue;
		}

		float DistanceSq = FVector::DistSquared(MyLocation, OverlappedActor->GetActorLocation());
		if (DistanceSq < MinDistanceSq)
		{
			MinDistanceSq = DistanceSq;
			ClosestEnemy = OverlappedActor;
		}
	}

	// 3. 상황에 따른 타겟 설정
	if (ClosestEnemy)
	{
		// 범위 내에 적군 유닛이 발견됨
		BBComp->SetValueAsObject(TargetActorKey.SelectedKeyName, ClosestEnemy);

		// 🚨 로그: 새로운 적 포착
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, FString::Printf(TEXT("[%s] 🎯 새 타겟 포착: %s"), *MyUnit->GetName(), *ClosestEnemy->GetName()));
	}
	else
	{
		// 범위 내에 적군 유닛이 없음 -> 적 기지로 타겟 복구
		AActor* EnemyBase = Cast<AActor>(BBComp->GetValueAsObject(EnemyBaseKey.SelectedKeyName));

		if (!EnemyBase)
		{
			// 블랙보드에 기지가 없다면 월드에서 검색하여 캐싱
			TArray<AActor*> FoundBases;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHomeBase::StaticClass(), FoundBases);

			for (AActor* BaseActor : FoundBases)
			{
				AHomeBase* HomeBase = Cast<AHomeBase>(BaseActor);
				// 기지도 적군 기지인지 확인 (IsEnemy 활용)
				if (HomeBase && MyUnit->IsEnemy(HomeBase))
				{
					EnemyBase = HomeBase;
					BBComp->SetValueAsObject(EnemyBaseKey.SelectedKeyName, EnemyBase);
					break;
				}
			}
		}

		BBComp->SetValueAsObject(TargetActorKey.SelectedKeyName, EnemyBase);


		// 🚨 로그: 주변에 적이 없어서 기지를 바라봄
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Silver, FString::Printf(TEXT("[%s] 🏠 주변 적 없음. 기지 타겟팅: %s"), *MyUnit->GetName(), EnemyBase ? *EnemyBase->GetName() : TEXT("None")));
	}
}