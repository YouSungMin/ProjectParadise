// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTTask_Attack.h"
#include "AIController.h"
#include "Characters/AIUnit/UnitBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/CrowdFollowingComponent.h"

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

	// 공격 전 회전 및 밀림 방지
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp)
	{
		// "TargetActor" 키 이름은 실제 BT 블랙보드 설정에 맞게 변경해야 할 수 있습니다.
		AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(FName("TargetActor")));
		if (TargetActor)
		{
			// 1. 군중 회피(Detour)로 인한 옆미끄러짐 및 모든 관성을 완벽하게 강제 정지
			AIController->StopMovement();

			// 2. 언리얼 AI 컨트롤러의 시선 고정 기능 사용 (이동 컴포넌트의 간섭 방지)
			AIController->SetFocus(TargetActor);

			// 3. 서서히 도는 것이 아니라 즉각적으로 팍! 하고 돌아보게 만들고 싶다면 기존 코드 병행
			FVector LookDir = TargetActor->GetActorLocation() - MyUnit->GetActorLocation();
			LookDir.Z = 0.f;
			MyUnit->SetActorRotation(LookDir.Rotation());
		}
	}

	// 🚨 [핵심 추가됨] 4. 공격 중 뒤 유닛에게 밀리지 않도록 Detour Crowd 회피 끄기 (바위처럼 고정)
	if (UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(AIController->GetPathFollowingComponent()))
	{
		CrowdComp->SuspendCrowdSteering(true);
	}

	UAbilitySystemComponent* ASC = MyUnit->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [공격 태스크] %s : ASC(어빌리티 컴포넌트)를 찾을 수 없습니다!"), *MyUnit->GetName());
		return EBTNodeResult::Failed;
	}

	FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Ability.Type.Basic"));

	// 어빌리티 발동 시도
	if (ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AttackTag)))
	{
		// 발동 성공
		UE_LOG(LogTemp, Log, TEXT("⚔️ [공격 태스크] %s : 어빌리티 발동 성공! (Succeeded)"), *MyUnit->GetName());
		return EBTNodeResult::Succeeded;
	}

	// 🚨 [로그 추가] 어빌리티 발동 실패 로그
	UE_LOG(LogTemp, Error, TEXT("❌ [공격 태스크] %s : 어빌리티 발동 실패!! (Failed) ➔ 쿨타임 중이거나 마나가 없거나, 태그가 잘못되었을 수 있습니다!"), *MyUnit->GetName());

	return EBTNodeResult::Failed;
}