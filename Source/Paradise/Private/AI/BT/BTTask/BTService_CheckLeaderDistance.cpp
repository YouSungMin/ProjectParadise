// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BTTask/BTService_CheckLeaderDistance.h"
#include "Perception/AIPerceptionComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Perception/AISense_Sight.h"

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
	if (!BB) return;

	//타겟 사망 여부 검사 (기존 코드 유지)
	AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetEnemy")));
	if (CurrentTarget)
	{
		AUnitBase* TargetUnit = Cast<AUnitBase>(CurrentTarget);
		if (TargetUnit && TargetUnit->IsDead())
		{
			BB->ClearValue(TEXT("TargetEnemy"));
			return; 
		}
	}

	//컨트롤러, 폰, 리더 검사
	AAIController* AICon = OwnerComp.GetAIOwner();
	APlayerBase* AICharacter = Cast<APlayerBase>(AICon ? AICon->GetPawn() : nullptr);
	AActor* Leader = Cast<AActor>(BB->GetValueAsObject(TEXT("LeaderActor")));

	//유효하지 않으면 즉시 종료
	if (!AICon || !AICharacter || !Leader) return;

	//리더와의 거리 계산
	float DistanceToLeader = FVector::Distance(AICharacter->GetActorLocation(), Leader->GetActorLocation());

	//거리가 충분히 가깝거나, 현재 때리고 있는 타겟이 없으면 리턴
	if (DistanceToLeader <= MaxChaseDistance || !CurrentTarget) return;


	//너무 멀어진 타겟 포기
	BB->ClearValue(TEXT("TargetEnemy"));

	UAIPerceptionComponent* PerceptionComp = AICon->GetPerceptionComponent();
	if (!PerceptionComp) return;

	//현재 시야에 들어와 있는 모든 액터를 가져옴
	TArray<AActor*> PerceivedActors;
	PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	//리더와 가까운 적이 있는지 하나씩 확인
	for (AActor* Actor : PerceivedActors)
	{
		// 자기 자신이거나 리더면 다음으로 넘어감
		if (Actor == AICharacter || Actor == Leader) continue;

		AUnitBase* PerceivedUnit = Cast<AUnitBase>(Actor);

		// 유닛이 아니거나, 이미 죽었으면 다음으로 넘어감
		if (!PerceivedUnit || PerceivedUnit->IsDead()) continue;

		// 적인지 태그로 판별 (아군이면 다음으로 넘어감)
		FGameplayTag EnemyTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"));
		if (!PerceivedUnit->GetFactionTag().MatchesTag(EnemyTag)) continue;

		// 이 적과 리더 사이의 거리를 계산
		float TargetDistToLeader = FVector::Distance(Actor->GetActorLocation(), Leader->GetActorLocation());

		// 리더와 가까운(추격 범위 내에 있는) 적이라면 즉시 새 타겟으로 지정!
		if (TargetDistToLeader <= MaxChaseDistance)
		{
			BB->SetValueAsObject(TEXT("TargetEnemy"), Actor);
			break; // 한 놈 찾았으니 더 안 찾아도 됨
		}
	}
}
