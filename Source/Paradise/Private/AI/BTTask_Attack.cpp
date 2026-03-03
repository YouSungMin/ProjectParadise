// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTTask_Attack.h"
#include "AIController.h"
#include "Characters/AIUnit/UnitBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack Target");
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AUnitBase* MyUnit = Cast<AUnitBase>(AIController ? AIController->GetPawn() : nullptr);

	if (!MyUnit) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = MyUnit->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;


	FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Ability.Type.Basic"));

	// 어빌리티 발동 시도
	if (ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AttackTag)))
	{
		// 발동 성공
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}