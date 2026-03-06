// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_FindLowestHPTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "EngineUtils.h"

UBTService_FindLowestHPTarget::UBTService_FindLowestHPTarget()
{
	NodeName = "Find Lowest HP Target (Range Unit)";
	Interval = 0.2f;
	SearchRadius = 1500.0f;
}

void UBTService_FindLowestHPTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	AUnitBase* SelfUnit = Cast<AUnitBase>(AIC->GetPawn());
	if (!SelfUnit) return;

	// 원거리 유닛의 현재 공격 사거리
	float AttackRange = SelfUnit->GetAttackRange();

	AUnitBase* BestTarget = nullptr;
	float MinHealth = FLT_MAX;

	for (TActorIterator<AUnitBase> It(GetWorld()); It; ++It)
	{
		AUnitBase* PotentialTarget = *It;

		// 1. 기본 검사
		if (!IsValid(PotentialTarget) || PotentialTarget == SelfUnit || PotentialTarget->IsDead()) continue;

		// 2. 적대 관계 확인
		if (SelfUnit->IsEnemy(PotentialTarget))
		{
			float Distance = FVector::Dist(SelfUnit->GetActorLocation(), PotentialTarget->GetActorLocation());

			// 3. 내 사거리(AttackRange) 안에 들어와 있는 유닛 검사
			if (Distance <= AttackRange)
			{
				// 4. GAS AttributeSet에서 체력 정보
				if (const UBaseAttributeSet* AS = Cast<UBaseAttributeSet>(PotentialTarget->GetAttributeSet()))
				{
					float CurrentHP = AS->GetHealth();

					// 5. 현재까지 찾은 유닛보다 체력이 더 적으면 교체
					if (CurrentHP < MinHealth)
					{
						MinHealth = CurrentHP;
						BestTarget = PotentialTarget;
					}
				}
			}
		}
	}

	if (BestTarget)
	{
		// 가장 낮은 HP를 가진 타겟을 블랙보드에 갱신
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, BestTarget);
		BB->SetValueAsFloat(FName("DistanceToTarget"), FVector::Dist(SelfUnit->GetActorLocation(), BestTarget->GetActorLocation()));
		BB->SetValueAsVector(FName("TargetLocation"), BestTarget->GetActorLocation());
	}
	else
	{
		// 사거리 안에 적 유닛이 없는 경우, 기존 타겟이 시야 밖으로 나갔는지 확인 후 처리
		UObject* RawTarget = BB->GetValueAsObject(TargetActorKey.SelectedKeyName);
		AUnitBase* CurrentTarget = Cast<AUnitBase>(RawTarget);

		// 현재 타겟이 없거나 죽었다면 기지로
		if (!IsValid(CurrentTarget) || CurrentTarget->IsDead())
		{
			BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
			BB->ClearValue(FName("DistanceToTarget"));

			AActor* DestBase = Cast<AActor>(BB->GetValueAsObject(FName("EnemyBaseActor")));
			if (DestBase)
			{
				BB->SetValueAsVector(FName("TargetLocation"), DestBase->GetActorLocation());
			}
		}
	}
}