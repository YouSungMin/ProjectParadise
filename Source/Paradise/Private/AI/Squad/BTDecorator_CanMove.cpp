// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Squad/BTDecorator_CanMove.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"

UBTDecorator_CanMove::UBTDecorator_CanMove()
{
	NodeName = "Can Move (Tag Check)";
}

bool UBTDecorator_CanMove::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AICon = OwnerComp.GetAIOwner())
	{
		if (ACharacterBase* Char = Cast<ACharacterBase>(AICon->GetPawn()))
		{
			// 캐릭터의 CanMove()가 true일 때만 이 노드의 실행을 허락합니다.
			return Char->CanMove();
		}
	}
	return false;
}
