// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTTaskNode_BossGASAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

UBTTaskNode_BossGASAttack::UBTTaskNode_BossGASAttack()
{
	NodeName = TEXT("Boss GAS Action (By Tag)");
}

EBTNodeResult::Type UBTTaskNode_BossGASAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return EBTNodeResult::Failed;

	APawn* BossPawn = AICon->GetPawn();
	if (!BossPawn) return EBTNodeResult::Failed;

	//적을 바라보게 회전
	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetKey.SelectedKeyName));
	if (Target)
	{
		FVector LookDir = Target->GetActorLocation() - BossPawn->GetActorLocation();
		LookDir.Z = 0.f;
		BossPawn->SetActorRotation(LookDir.Rotation());
	}

	//ASC 가져오기
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(BossPawn);

	if (ASC && AbilityTagToActivate.IsValid())
	{
		//에디터에서 세팅한 태그 어빌리티 발동
		bool bSuccess = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTagToActivate));

		if (bSuccess)
		{
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
