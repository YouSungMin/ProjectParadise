// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_FindClosestTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "EngineUtils.h"

UBTService_FindClosestTarget::UBTService_FindClosestTarget()
{
	NodeName = "Find Closest Target";
	Interval = 0.2f;
	SearchRadius = 1500.0f;
}

void UBTService_FindClosestTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;

	ACharacterBase* SelfChar = Cast<ACharacterBase>(AIC->GetPawn());
	if (!SelfChar) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	// 1. 현재 전투 타겟 유효성 검사
	ACharacterBase* CurrentTarget = Cast<ACharacterBase>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (CurrentTarget && !CurrentTarget->IsDead())
	{
		float CurrentDistance = FVector::Dist(SelfChar->GetActorLocation(), CurrentTarget->GetActorLocation());
		BB->SetValueAsFloat(FName("DistanceToTarget"), CurrentDistance);

		// 이미 싸울 대상이 있다면 로직 종료 (기지 좌표 갱신 안 함 - 전투 집중)
		return;
	}

	// 2. 새로운 적 유닛 탐색
	ACharacterBase* ClosestEnemy = nullptr;
	float MinDistance = SearchRadius;

	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		ACharacterBase* OtherChar = *It;

		if (!OtherChar || OtherChar == SelfChar || OtherChar->IsDead()) continue;

		// 피아식별 (아군은 적군을, 적군은 아군을 찾음)
		if (SelfChar->IsHostile(OtherChar))
		{
			float Distance = FVector::Dist(SelfChar->GetActorLocation(), OtherChar->GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestEnemy = OtherChar;
			}
		}
	}

	// 3. 탐색 결과 처리 및 기지 좌표 동기화
	if (ClosestEnemy)
	{
		// 적 유닛 발견 시 해당 유닛을 타겟으로 설정
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, ClosestEnemy);
		BB->SetValueAsFloat(FName("DistanceToTarget"), MinDistance);
	}
	else
	{
		// 적 유닛이 주변에 없는 경우 (진격 모드)
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
		BB->SetValueAsFloat(FName("DistanceToTarget"), 999999.0f);

		// 스포너에서 넣어준 '상대방 기지' 정보 확인
		AActor* BaseActor = Cast<AActor>(BB->GetValueAsObject(FName("EnemyBaseActor")));

		if (BaseActor)
		{
			FVector TargetLoc = BaseActor->GetActorLocation();

			// 모든 이동 관련 블랙보드 키를 기지 위치로 강제 동기화
			BB->SetValueAsVector(FName("TargetLocation"), TargetLoc);
			BB->SetValueAsVector(FName("MoveLocation"), TargetLoc);

			// BT 노드 호환성을 위해 HomeBaseActor에도 값 할당
			BB->SetValueAsObject(FName("HomeBaseActor"), BaseActor);
		}
	}
}