// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_FindClosestTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "EngineUtils.h"

UBTService_FindClosestTarget::UBTService_FindClosestTarget()
{
	NodeName = "Find Closest Target";
	Interval = 0.1f;
	SearchRadius = 1500.0f;
}

void UBTService_FindClosestTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	AUnitBase* SelfUnit = Cast<AUnitBase>(AIC->GetPawn());
	if (!SelfUnit) return;

	ACharacterBase* CurrentTarget = Cast<ACharacterBase>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (CurrentTarget && !CurrentTarget->IsDead())
	{
		float Distance = FVector::Dist(SelfUnit->GetActorLocation(), CurrentTarget->GetActorLocation());
		BB->SetValueAsFloat(FName("DistanceToTarget"), Distance);
		return;
	}

	ACharacterBase* ClosestEnemy = nullptr;
	float MinDistance = SearchRadius;

	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		ACharacterBase* OtherChar = *It;
		if (!OtherChar || OtherChar == SelfUnit || OtherChar->IsDead()) continue;

		if (SelfUnit->IsHostile(OtherChar))
		{
			float Distance = FVector::Dist(SelfUnit->GetActorLocation(), OtherChar->GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestEnemy = OtherChar;
			}
		}
	}

	if (ClosestEnemy)
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, ClosestEnemy);
		BB->SetValueAsFloat(FName("DistanceToTarget"), MinDistance);
	}
	else
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);

		AActor* DestBase = Cast<AActor>(BB->GetValueAsObject(FName("EnemyBaseActor")));
		if (DestBase)
		{
			FVector BaseLoc = DestBase->GetActorLocation();
			BB->SetValueAsVector(FName("TargetLocation"), BaseLoc);
			BB->SetValueAsVector(FName("MoveLocation"), BaseLoc);
		}
	}
}