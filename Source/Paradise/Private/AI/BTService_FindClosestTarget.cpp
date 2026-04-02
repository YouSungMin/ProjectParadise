// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_FindClosestTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"

UBTService_FindClosestTarget::UBTService_FindClosestTarget()
{
	NodeName = "Find Closest Target";
	Interval = 0.2f;
	SearchRadius = 1500.0f;
}

void UBTService_FindClosestTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	AUnitBase* SelfUnit = Cast<AUnitBase>(AIC->GetPawn());
	if (!SelfUnit) return;

	UObject* RawTarget = BB->GetValueAsObject(TargetActorKey.SelectedKeyName);
	ACharacterBase* CurrentTarget = Cast<ACharacterBase>(RawTarget);

	// [1단계] 현재 타겟이 유효한지 체크
	if (IsValid(CurrentTarget) && !CurrentTarget->IsDead())
	{
		float CenterDist = FVector::Dist(SelfUnit->GetActorLocation(), CurrentTarget->GetActorLocation());
		float MyRadius = SelfUnit->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float TargetRadius = CurrentTarget->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float SurfaceDist = FMath::Max(0.0f, CenterDist - MyRadius - TargetRadius);

		float MyAttackRange = SelfUnit->GetAttackRange();

		if (SurfaceDist < SearchRadius)
		{
			BB->SetValueAsFloat(FName("DistanceToTarget"), SurfaceDist);

			if (SurfaceDist <= MyAttackRange)
			{
				BB->SetValueAsVector(FName("TargetLocation"), SelfUnit->GetActorLocation());
			}
			else
			{
				// 사거리 밖이라면 적을 추적합니다.
				BB->SetValueAsVector(FName("TargetLocation"), CurrentTarget->GetActorLocation());
			}

			return;
		}
	}

	// [2단계] 새로운 적 탐색 (기존 코드와 동일)
	ACharacterBase* ClosestEnemy = nullptr;
	float MinDistance = SearchRadius;

	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		ACharacterBase* OtherChar = *It;
		if (OtherChar->IsA<AHomeBase>()) continue;
		if (!IsValid(OtherChar) || OtherChar == SelfUnit || OtherChar->IsDead()) continue;

		if (SelfUnit->IsHostile(OtherChar))
		{
			float CenterToCenterDistance = FVector::Dist(SelfUnit->GetActorLocation(), OtherChar->GetActorLocation());
			float MyRadius = SelfUnit->GetCapsuleComponent()->GetScaledCapsuleRadius();
			float TargetRadius = OtherChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
			float SurfaceDistance = FMath::Max(0.0f, CenterToCenterDistance - MyRadius - TargetRadius);

			if (SurfaceDistance < MinDistance)
			{
				MinDistance = SurfaceDistance;
				ClosestEnemy = OtherChar;
			}
		}
	}

	// [3단계] 탐색 결과 반영
	if (ClosestEnemy)
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, ClosestEnemy);
		BB->SetValueAsFloat(FName("DistanceToTarget"), MinDistance);

		// 🚀 [수정] 처음 발견했을 때도 사거리 체크를 해서 목적지를 결정합니다.
		float MyAttackRange = SelfUnit->GetAttackRange();
		if (MinDistance <= MyAttackRange)
		{
			// 이미 사거리 안이면 발견 즉시 그 자리에 멈춤
			BB->SetValueAsVector(FName("TargetLocation"), SelfUnit->GetActorLocation());
		}
		else
		{
			// 사거리 밖이면 적을 향해 이동
			BB->SetValueAsVector(FName("TargetLocation"), ClosestEnemy->GetActorLocation());
		}
	}
	else
	{
		// [4단계] 주변에 적 유닛이 없는 경우 (기지 이동 로직)
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
		BB->ClearValue(FName("DistanceToTarget"));

		FVector CurrentTargetLoc = BB->GetValueAsVector(FName("TargetLocation"));
		AActor* DestBase = Cast<AActor>(BB->GetValueAsObject(FName("EnemyBaseActor")));
		
		if (DestBase && CurrentTargetLoc != FVector::ZeroVector)
		{
			// 현재 찍힌 목적지와 기지 사이의 거리를 체크 (800~1000 유닛 이내면 이미 기지 근처임)
			float DistToBase = FVector::Dist(CurrentTargetLoc, DestBase->GetActorLocation());
			
			if (DistToBase < 1000.0f)
			{
				return;
			}
		}

		// 위 조건에 걸리지 않으면(목적지가 없거나 너무 멀면) 아래에서 새로 목적지를 잡습니다.
		if (DestBase)
		{
			FVector BaseLocation = DestBase->GetActorLocation();
			FVector RandomOffset = FMath::VRand();
			RandomOffset.Z = 0.0f;
			RandomOffset.Normalize();

			float RandomRadius = FMath::RandRange(300.0f, 700.0f);
			FVector FinalTargetLocation = BaseLocation + (RandomOffset * RandomRadius);

			BB->SetValueAsVector(FName("TargetLocation"), FinalTargetLocation);
		}
	}
}