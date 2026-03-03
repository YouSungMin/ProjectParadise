// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_CheckTargetDeath.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/AIUnit/UnitBase.h"
#include "AIController.h"

UBTService_CheckTargetDeath::UBTService_CheckTargetDeath()
{
	NodeName = "Check Target Death";
	Interval = 0.5f; // 0.5초마다 체크
	RandomDeviation = 0.1f;
}

void UBTService_CheckTargetDeath::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	// 1. 블랙보드에서 타겟 액터를 가져옴
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (TargetActor)
	{
		// 2. 타겟이 BaseUnit인지 확인하고 죽었는지 체크
		AUnitBase* UnitBase = Cast<AUnitBase>(TargetActor);
		if (UnitBase && UnitBase->IsDead())
		{
			// 3. 죽었다면 블랙보드 키를 비워줌
			BB->ClearValue(TargetActorKey.SelectedKeyName);
			UE_LOG(LogTemp, Log, TEXT("Target is Dead. Clearing Blackboard Key."));
		}
	}
}