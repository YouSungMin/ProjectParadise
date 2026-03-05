// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/InGame/MyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AI/MonsterAI.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Core/ParadiseGameInstance.h"

AMyAIController::AMyAIController()
{
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	if (SightConfig)
	{
		SightConfig->SightRadius = 800.f;
		SightConfig->LoseSightRadius = 1000.f;
		SightConfig->PeripheralVisionAngleDegrees = 90.f;
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		if (AIPerception)
		{
			AIPerception->ConfigureSense(*SightConfig);
			AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
		}
	}

	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AMyAIController::OnTargetDetected);
	}
}

void AMyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    UBlackboardComponent* BBComp = Blackboard.Get();
    if (BTAsset && BBAsset && UseBlackboard(BBAsset, BBComp))
    {
        Blackboard = BBComp;
        AUnitBase* SelfUnit = Cast<AUnitBase>(InPawn);
        if (SelfUnit)
        {
            // 1. 사거리 데이터 로드
            Blackboard->SetValueAsFloat(TEXT("TargetAttackRange"), SelfUnit->GetAttackRange());

            // 2. 적대적인 기지 찾기
            TArray<AActor*> FoundBases;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHomeBase::StaticClass(), FoundBases);

            UE_LOG(LogTemp, Log, TEXT("🔍 [%s] 주변 기지 검색 시작 (총 %d개 발견)"), *SelfUnit->GetName(), FoundBases.Num());

            for (AActor* Actor : FoundBases)
            {
                AHomeBase* HomeBase = Cast<AHomeBase>(Actor);
                if (HomeBase)
                {
                    FGameplayTag MyTag = SelfUnit->GetFactionTag();
                    FGameplayTag BaseTag = HomeBase->GetFactionTag();
                    bool bIsEnemyResult = SelfUnit->IsEnemy(HomeBase);

                    UE_LOG(LogTemp, Warning, TEXT("   👉 [Check] 나: %s(%s) vs 기지: %s(%s) | 적대판정: %s"),
                        *SelfUnit->GetName(),
                        *MyTag.ToString(),
                        *HomeBase->GetName(),
                        *BaseTag.ToString(),
                        bIsEnemyResult ? TEXT("TRUE (공격대상)") : TEXT("FALSE (아군/중립)")
                    );

                    if (bIsEnemyResult)
                    {
                        Blackboard->SetValueAsObject(TEXT("EnemyBaseActor"), HomeBase);
                        UE_LOG(LogTemp, Error, TEXT("🚀 [%s] 타겟 확정! 공격하러 갑니다 -> %s"), *SelfUnit->GetName(), *HomeBase->GetName());
                        break;
                    }
                }
            }
        }

        RunBehaviorTree(BTAsset);
    }
}

void AMyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
    if (Blackboard == nullptr || Actor == nullptr) return;

    if (Actor->IsHidden())
    {
        AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_KEYS::TargetActor));
        if (CurrentTarget == Actor)
        {
            Blackboard->ClearValue(BB_KEYS::TargetActor);
        }
        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        AUnitBase* TargetUnit = Cast<AUnitBase>(Actor);
        AUnitBase* SelfUnit = Cast<AUnitBase>(GetPawn());

        if (TargetUnit && SelfUnit && SelfUnit->IsEnemy(TargetUnit))
        {
            // 현재 타겟이 이미 있다면, 시야에 새로운 적이 들어와도 무시
            AActor* ExistingTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_KEYS::TargetActor));
            if (!IsValid(ExistingTarget) || (Cast<ACharacterBase>(ExistingTarget) && Cast<ACharacterBase>(ExistingTarget)->IsDead()))
            {
                Blackboard->SetValueAsObject(BB_KEYS::TargetActor, Actor);
            }
        }
    }
    else
    {
        // 시야에서 완전히 사라졌을 때만 타겟 해제
        AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_KEYS::TargetActor));
        if (CurrentTarget == Actor)
        {
            Blackboard->ClearValue(BB_KEYS::TargetActor);
        }
    }
}