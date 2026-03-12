// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_FindLowestHPTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "EngineUtils.h"

UBTService_FindLowestHPTarget::UBTService_FindLowestHPTarget()
{
    NodeName = "Find Lowest HP Percent Target";
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

    float AttackRange = SelfUnit->GetAttackRange();

    AUnitBase* BestUnitTarget = nullptr;
    float MinUnitHealthPercent = 2.0f;

    AUnitBase* BestBaseTarget = nullptr;
    float MinBaseHealthPercent = 2.0f;

	//0312 김성현 - 탐색 로직 Iterator에서 overlap으로 변경

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(SelfUnit); // 자기 자신은 검색에서 제외

	// 탐색할 오브젝트 타입 (유닛은 Pawn, 기지는 WorldDynamic일 확률이 높으므로 둘 다 추가)
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// 내 위치를 중심으로 'AttackRange(사거리)' 만큼의 구체를 그려서 겹치는 놈들만 가져옵니다!
	bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		SelfUnit->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(AttackRange),
		CollisionParams
	);

	if (bHit)
	{
		for (const FOverlapResult& Hit : OverlapResults)
		{
			AUnitBase* PotentialTarget = Cast<AUnitBase>(Hit.GetActor());

			// 유효성 및 사망 체크
			if (!IsValid(PotentialTarget) || PotentialTarget->IsDead()) continue;

			// 적군인지 확인
			if (SelfUnit->IsEnemy(PotentialTarget))
			{
				// 💡 오버랩 자체가 AttackRange 반경 안에서만 일어났으므로 거리 검사(Distance <= AttackRange)는 생략해도 무방합니다!
				const UBaseAttributeSet* AS = Cast<UBaseAttributeSet>(PotentialTarget->GetAttributeSet());
				if (AS)
				{
					float CurrentHP = AS->GetHealth();
					float MaxHP = AS->GetMaxHealth();
					float HealthPercent = (MaxHP > 0.0f) ? (CurrentHP / MaxHP) : 1.0f;

					if (PotentialTarget->IsA(AHomeBase::StaticClass()))
					{
						if (HealthPercent < MinBaseHealthPercent)
						{
							MinBaseHealthPercent = HealthPercent;
							BestBaseTarget = PotentialTarget;
						}
					}
					else
					{
						if (HealthPercent < MinUnitHealthPercent)
						{
							MinUnitHealthPercent = HealthPercent;
							BestUnitTarget = PotentialTarget;
						}
					}
				}
			}
		}
	}
	// =========================================================================

	// 유닛 우선순위 적용
	AUnitBase* FinalTarget = (BestUnitTarget != nullptr) ? BestUnitTarget : BestBaseTarget;

	// 2. 최종 타겟팅 결과에 따른 블랙보드 갱신
	if (FinalTarget)
	{
		float DistanceToTarget = FVector::Dist(SelfUnit->GetActorLocation(), FinalTarget->GetActorLocation());

		// 타겟 액터와 실시간 거리는 항상 업데이트 (타겟팅 전환용)
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, FinalTarget);
		BB->SetValueAsFloat(FName("DistanceToTarget"), DistanceToTarget);

		if (DistanceToTarget <= AttackRange)
		{
			// 이미 사거리 안이라면 내 현재 위치를 목표로 설정하여 이동 중지 유도
			BB->SetValueAsVector(FName("TargetLocation"), SelfUnit->GetActorLocation());
		}
		else
		{
			// 사거리 밖으로 나가면 다시 적을 추적하기 위해 위치 갱신
			BB->SetValueAsVector(FName("TargetLocation"), FinalTarget->GetActorLocation());
		}
	}
	else
	{
		// 사거리 내에 타겟이 없는 경우: 기존 타겟이 죽었는지 확인
		UObject* RawTarget = BB->GetValueAsObject(TargetActorKey.SelectedKeyName);
		AUnitBase* CurrentTarget = Cast<AUnitBase>(RawTarget);

		if (!IsValid(CurrentTarget) || CurrentTarget->IsDead())
		{
			BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
			BB->ClearValue(FName("DistanceToTarget"));

			// 사거리 내에 유닛이 없으므로 적 기지로 이동 좌표 설정
			AActor* DestBase = Cast<AActor>(BB->GetValueAsObject(FName("EnemyBaseActor")));
			if (DestBase)
			{
				BB->SetValueAsVector(FName("TargetLocation"), DestBase->GetActorLocation());
			}
		}
	}

    //// 1. 사거리 내의 모든 적(유닛/기지)을 탐색하여 최저 HP% 타겟 선정
    //for (TActorIterator<AUnitBase> It(GetWorld()); It; ++It)
    //{
    //    AUnitBase* PotentialTarget = *It;

    //    if (!IsValid(PotentialTarget) || PotentialTarget == SelfUnit || PotentialTarget->IsDead()) continue;

    //    if (SelfUnit->IsEnemy(PotentialTarget))
    //    {
    //        float Distance = FVector::Dist(SelfUnit->GetActorLocation(), PotentialTarget->GetActorLocation());

    //        // 사거리 내에 있을 때만 비교 대상에 포함
    //        if (Distance <= AttackRange)
    //        {
    //            const UBaseAttributeSet* AS = Cast<UBaseAttributeSet>(PotentialTarget->GetAttributeSet());
    //            if (AS)
    //            {
    //                float CurrentHP = AS->GetHealth();
    //                float MaxHP = AS->GetMaxHealth();
    //                float HealthPercent = (MaxHP > 0.0f) ? (CurrentHP / MaxHP) : 1.0f;

    //                if (PotentialTarget->IsA(AHomeBase::StaticClass()))
    //                {
    //                    if (HealthPercent < MinBaseHealthPercent)
    //                    {
    //                        MinBaseHealthPercent = HealthPercent;
    //                        BestBaseTarget = PotentialTarget;
    //                    }
    //                }
    //                else
    //                {
    //                    if (HealthPercent < MinUnitHealthPercent)
    //                    {
    //                        MinUnitHealthPercent = HealthPercent;
    //                        BestUnitTarget = PotentialTarget;
    //                    }
    //                }
    //            }
    //        }
    //    }
    //}

    //// 유닛 우선순위 적용
    //AUnitBase* FinalTarget = (BestUnitTarget != nullptr) ? BestUnitTarget : BestBaseTarget;

    //// 2. 최종 타겟팅 결과에 따른 블랙보드 갱신
    //if (FinalTarget)
    //{
    //    float DistanceToTarget = FVector::Dist(SelfUnit->GetActorLocation(), FinalTarget->GetActorLocation());

    //    // 타겟 액터와 실시간 거리는 항상 업데이트 (타겟팅 전환용)
    //    BB->SetValueAsObject(TargetActorKey.SelectedKeyName, FinalTarget);
    //    BB->SetValueAsFloat(FName("DistanceToTarget"), DistanceToTarget);

    //    if (DistanceToTarget <= AttackRange)
    //    {
    //        // 이미 사거리 안이라면 내 현재 위치를 목표로 설정하여 이동 중지 유도
    //        BB->SetValueAsVector(FName("TargetLocation"), SelfUnit->GetActorLocation());
    //    }
    //    else
    //    {
    //        // 사거리 밖으로 나가면 다시 적을 추적하기 위해 위치 갱신
    //        BB->SetValueAsVector(FName("TargetLocation"), FinalTarget->GetActorLocation());
    //    }
    //}
    //else
    //{
    //    // 사거리 내에 타겟이 없는 경우: 기존 타겟이 죽었는지 확인
    //    UObject* RawTarget = BB->GetValueAsObject(TargetActorKey.SelectedKeyName);
    //    AUnitBase* CurrentTarget = Cast<AUnitBase>(RawTarget);

    //    if (!IsValid(CurrentTarget) || CurrentTarget->IsDead())
    //    {
    //        BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
    //        BB->ClearValue(FName("DistanceToTarget"));

    //        // 사거리 내에 유닛이 없으므로 적 기지로 이동 좌표 설정
    //        AActor* DestBase = Cast<AActor>(BB->GetValueAsObject(FName("EnemyBaseActor")));
    //        if (DestBase)
    //        {
    //            BB->SetValueAsVector(FName("TargetLocation"), DestBase->GetActorLocation());
    //        }
    //    }
    //}
}