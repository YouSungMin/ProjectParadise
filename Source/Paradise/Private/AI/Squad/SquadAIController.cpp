// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Squad/SquadAIController.h"
#include "Characters/AIUnit/UnitBase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Characters/Base/PlayerBase.h"


ASquadAIController::ASquadAIController()
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
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ASquadAIController::OnTargetDetected);
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
	UE_LOG(LogTemp, Warning, TEXT("🗡️ [SquadAI] 적 발견!"));

	if (!Blackboard || !Actor) return;

	// 리더(플레이어)이거나 자기 자신이면 타겟팅 무시
	if (Actor == Blackboard->GetValueAsObject(FName("LeaderActor")) || Actor == GetPawn())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		UE_LOG(LogTemp, Warning, TEXT("🗡️ [SquadAI] Stimulus : 적 발견!"));
		// 감지된 액터가 유닛(몬스터/퍼밀리어 등)인지 확인
		AUnitBase* TargetUnit = Cast<AUnitBase>(Actor);
		if (TargetUnit)
		{
			//타겟의 진영 태그 가져오기 
			FGameplayTag TargetFaction = TargetUnit->GetFactionTag();

			//적인지 판별
			FGameplayTag EnemyTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"));
			if (TargetFaction.MatchesTag(EnemyTag))
			{
				Blackboard->SetValueAsObject(FName("TargetEnemy"), Actor);
				UE_LOG(LogTemp, Warning, TEXT("🗡️ [SquadAI] 적 발견! 타겟 설정: %s (태그: %s)"), *Actor->GetName(), *TargetFaction.ToString());
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