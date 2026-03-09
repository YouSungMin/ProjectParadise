// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTTaskNode_BossGASAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/AIUnit/UnitBase.h"

UBTTaskNode_BossGASAttack::UBTTaskNode_BossGASAttack()
{
	NodeName = TEXT("Boss GAS Action");
}

EBTNodeResult::Type UBTTaskNode_BossGASAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AUnitBase* Unit = Cast<AUnitBase>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Unit) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor")));

	if (Target)
	{
		FVector LookDir = Target->GetActorLocation() - Unit->GetActorLocation();
		LookDir.Z = 0.f; // 위아래로 기울어지는 것 방지
		Unit->SetActorRotation(LookDir.Rotation());
	}

	if (ActionInputID == EInputID::Attack)
	{
		Unit->GetAbilitySystemComponent();
	}


	return EBTNodeResult::Type();
}
