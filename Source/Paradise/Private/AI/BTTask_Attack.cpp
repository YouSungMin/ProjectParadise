// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTTask_Attack.h"
#include "AIController.h"
#include "Characters/AIUnit/UnitBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack Target");
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed; // 방어 로직 추가

	AUnitBase* MyUnit = Cast<AUnitBase>(AIController->GetPawn());
	if (!MyUnit) return EBTNodeResult::Failed;

	//공격전 회전
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp)
	{
		// "TargetActor" 키 이름은 실제 BT 블랙보드 설정에 맞게 변경해야 할 수 있습니다.
		AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(FName("TargetActor")));
		if (TargetActor)
		{
			// 🚨 1. 군중 회피(Detour)로 인한 옆미끄러짐 및 모든 관성을 완벽하게 강제 정지
			AIController->StopMovement();

			// 🚨 2. 언리얼 AI 컨트롤러의 시선 고정 기능 사용 (이동 컴포넌트의 간섭 방지)
			AIController->SetFocus(TargetActor);

			// 3. 서서히 도는 것이 아니라 즉각적으로 팍! 하고 돌아보게 만들고 싶다면 기존 코드 병행
			FVector LookDir = TargetActor->GetActorLocation() - MyUnit->GetActorLocation();
			LookDir.Z = 0.f;
			MyUnit->SetActorRotation(LookDir.Rotation());
		}
	}
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