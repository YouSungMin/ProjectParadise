// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BTTask/BTTask_SquadGASAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Base/PlayerBase.h"
#include "Data/Enums/GameEnums.h"

UBTTask_SquadGASAttack::UBTTask_SquadGASAttack()
{
	NodeName = TEXT("Squad GAS Attack");
}

EBTNodeResult::Type UBTTask_SquadGASAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	//현재 AI가 조종 중인 캐릭터 가져오기
	APlayerBase* SquadPawn = Cast<APlayerBase>(OwnerComp.GetAIOwner()->GetPawn());
	if (!SquadPawn) return EBTNodeResult::Failed;

	//적을 자연스럽게 바라보게 만들기
	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetEnemy")));
	if (Target)
	{
		FVector LookDir = Target->GetActorLocation() - SquadPawn->GetActorLocation();
		LookDir.Z = 0.f; // 위아래로 기울어지는 것 방지
		SquadPawn->SetActorRotation(LookDir.Rotation());
	}

	//UI에서 버튼 누른 것과 완벽히 동일하게 GAS 평타 어빌리티 실행
	SquadPawn->SendAbilityInputToASC(EInputID::Attack, true);

	//공격 명령을 내렸으니 Task 성공 반환
	return EBTNodeResult::Succeeded;
}
