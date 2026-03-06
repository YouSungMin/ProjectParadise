// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Squad/SquadAIController.h"
#include "Characters/AIUnit/UnitBase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Characters/Base/PlayerBase.h"

ASquadAIController::ASquadAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	//시야 감지 
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	if (SightConfig)
	{
		SightConfig->SightRadius = 1000.f;     // 적 탐색 반경 
		SightConfig->LoseSightRadius = 1200.f; // 시야에서 놓치는 반경
		SightConfig->PeripheralVisionAngleDegrees = 180.f; // 360도 전방위 감지
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = false; // 아군은 기본 무시

		AIPerception->ConfigureSense(*SightConfig);
		AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
	}
}



void ASquadAIController::SetLeader(AActor* CurrentLeaderActor)
{
	if (Blackboard)
	{
		Blackboard->SetValueAsObject(FName("LeaderActor"), CurrentLeaderActor);
		UE_LOG(LogTemp, Log, TEXT("🤖 [SquadAI] 리더 타겟 갱신: %s"), *CurrentLeaderActor->GetName());
	}
}


void ASquadAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (AIPerception)
	{
		UE_LOG(LogTemp, Log, TEXT("🤖 [SquadAI] AIPerception 바인딩 성공"));
		AIPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ASquadAIController::OnTargetDetected);
	}

	UBlackboardComponent* BBComp = Blackboard.Get();
	if (BTAsset && BBAsset && UseBlackboard(BBAsset, BBComp))
	{
		Blackboard = BBComp;

		// (선택) 무기 사거리 등을 PlayerBase에서 가져와서 블랙보드에 넣을 수 있습니다.
		// Blackboard->SetValueAsFloat(TEXT("TargetAttackRange"), 200.f);

		RunBehaviorTree(BTAsset);
		UE_LOG(LogTemp, Log, TEXT("🤖 [SquadAI] 동료 AI 비헤이비어 트리 가동 시작!"));
	}
}


void ASquadAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{

	if (!Blackboard || !Actor) return;

	// 리더(플레이어)이거나 자기 자신이면 타겟팅 무시
	if (Actor == Blackboard->GetValueAsObject(FName("LeaderActor")) || Actor == GetPawn())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		AUnitBase* TargetUnit = Cast<AUnitBase>(Actor);
		if (TargetUnit)
		{
			FGameplayTag TargetFaction = TargetUnit->GetFactionTag();
			FGameplayTag EnemyTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"));

			if (TargetFaction.MatchesTag(EnemyTag))
			{
				//이미 블랙보드에 저장된 타겟이 있는지 확인
				AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(FName("TargetEnemy")));

				//기존 타겟이 존재하고, 그 타겟이 방금 새로 감지된 액터와 다르다면
				if (CurrentTarget && CurrentTarget != Actor)
				{
					AUnitBase* CurrentTargetUnit = Cast<AUnitBase>(CurrentTarget);

					//기존 타겟이 아직 살아있다면, 새 타겟으로 바꾸지 않고 무시
					if (CurrentTargetUnit && !CurrentTargetUnit->IsDead())
					{
						return;
					}
				}

				// 기존 타겟이 없거나, 죽었을 때만 새로운 타겟을 설정
				Blackboard->SetValueAsObject(FName("TargetEnemy"), Actor);
				UE_LOG(LogTemp, Warning, TEXT("🗡️ [SquadAI] 새로운 타겟 고정: %s"), *Actor->GetName());
			}
		}
	}
	else
	{
		// 시야에서 벗어나면 타겟 초기화
		AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(FName("TargetEnemy")));
		if (CurrentTarget == Actor)
		{
			Blackboard->ClearValue(FName("TargetEnemy"));
		}
	}
}