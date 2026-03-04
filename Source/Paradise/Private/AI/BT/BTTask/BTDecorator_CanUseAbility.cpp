// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BTTask/BTDecorator_CanUseAbility.h"
#include "AIController.h"
#include "Characters/Base/PlayerBase.h"
#include "AbilitySystemComponent.h"

UBTDecorator_CanUseAbility::UBTDecorator_CanUseAbility()
{
	NodeName = TEXT("Can Use GAS Ability?");
}

bool UBTDecorator_CanUseAbility::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	APlayerBase* SquadPawn = Cast<APlayerBase>(OwnerComp.GetAIOwner()->GetPawn());
	if (!SquadPawn || !SquadPawn->GetAbilitySystemComponent()) return false;

	UAbilitySystemComponent* ASC = SquadPawn->GetAbilitySystemComponent();

	//ASC의 부여된 어빌리티를 검색
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		//InputID 가 찾는 종류의 ID 인지 검사
		if (Spec.InputID == static_cast<int32>(ActionInputID))
		{
			//GAS가 스스로 마나와 쿨타임을 계산해서 쓸 수 있는지(True/False) 반환
			return Spec.Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get());
		}
	}
	return false;
}
