// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BTTask/BTService_CheckLeaderDistance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Base/PlayerBase.h"

UBTService_CheckLeaderDistance::UBTService_CheckLeaderDistance()
{
	NodeName = TEXT("Check Leader Distance");
	Interval = 0.5f; // 0.5초마다 검사
	RandomDeviation = 0.1f;
}

void UBTService_CheckLeaderDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	APlayerBase* AICharacter = Cast<APlayerBase>(OwnerComp.GetAIOwner()->GetPawn());
	AActor* Leader = Cast<AActor>(BB->GetValueAsObject(TEXT("LeaderActor")));

	if (AICharacter && Leader)
	{
		float DistanceToLeader = FVector::Distance(AICharacter->GetActorLocation(), Leader->GetActorLocation());

		// 리더와 너무 멀어졌고, 현재 타겟을 때리고 있는 중이라면
		if (DistanceToLeader > MaxChaseDistance && BB->GetValueAsObject(TEXT("TargetEnemy")) != nullptr)
		{
			// 타겟을 없애고 리더로 돌아오게끔 함
			BB->ClearValue(TEXT("TargetEnemy"));
			UE_LOG(LogTemp, Warning, TEXT("🏃 [SquadAI] 리더와 너무 멉니다! 타겟을 포기하고 복귀합니다."));
		}
	}
}
