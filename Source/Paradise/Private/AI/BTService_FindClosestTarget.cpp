// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_FindClosestTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "EngineUtils.h"

UBTService_FindClosestTarget::UBTService_FindClosestTarget()
{
	NodeName = "Find Closest Target";
	Interval = 0.2f;
	SearchRadius = 1500.0f;
}

void UBTService_FindClosestTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	AUnitBase* SelfUnit = Cast<AUnitBase>(AIC->GetPawn());
	if (!SelfUnit) return;

	// [1단계] 현재 블랙보드에 저장된 타겟이 '유효'한지 체크
	// Object 타입의 키값을 가져와서 CharacterBase로 캐스팅합니다.
	UObject* RawTarget = BB->GetValueAsObject(TargetActorKey.SelectedKeyName);
	ACharacterBase* CurrentTarget = Cast<ACharacterBase>(RawTarget);

	// 타겟이 존재하고(IsValid), 살아있다면(!IsDead) 
	// 주변에 더 가까운 적이 있는지 검색조차 하지 않고 즉시 리턴합니다.
	if (IsValid(CurrentTarget) && !CurrentTarget->IsDead())
	{
		// 위치와 거리 정보만 최신화 (이동 및 공격 거리 체크용)
		float Distance = FVector::Dist(SelfUnit->GetActorLocation(), CurrentTarget->GetActorLocation());
		if (Distance < (SearchRadius * 1.0f))
		{
			BB->SetValueAsFloat(FName("DistanceToTarget"), Distance);
			BB->SetValueAsVector(FName("TargetLocation"), CurrentTarget->GetActorLocation());

			return;
		}
	}

	// [2단계] 타겟이 없거나 죽은 경우에만 아래의 '새로운 적 탐색' 로직 실행
	ACharacterBase* ClosestEnemy = nullptr;
	float MinDistance = SearchRadius;

	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		ACharacterBase* OtherChar = *It;

		// 기본 유효성 검사 (본인 제외, 유효성, 사망 여부)
		if (!IsValid(OtherChar) || OtherChar == SelfUnit || OtherChar->IsDead()) continue;

		// 유닛의 적대 관계 확인 (Faction Tag 기반 등)
		if (SelfUnit->IsHostile(OtherChar))
		{
			float Distance = FVector::Dist(SelfUnit->GetActorLocation(), OtherChar->GetActorLocation());

			// 설정된 SearchRadius(1500) 내에서 가장 가까운 적을 찾음
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestEnemy = OtherChar;
			}
		}
	}

	// [3단계] 탐색 결과 반영
	if (ClosestEnemy)
	{
		// 새로운 적을 발견함 (이제 이 적이 죽을 때까지 1단계에서 걸러짐)
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, ClosestEnemy);
		BB->SetValueAsFloat(FName("DistanceToTarget"), MinDistance);
		BB->SetValueAsVector(FName("TargetLocation"), ClosestEnemy->GetActorLocation());
	}
	else
	{
		// [4단계] 주변에 적 유닛이 아무도 없는 경우
		// 타겟 키를 비우고, 적 기지(HomeBase)를 최종 목적지로 설정
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
		BB->ClearValue(FName("DistanceToTarget"));

		AActor* DestBase = Cast<AActor>(BB->GetValueAsObject(FName("EnemyBaseActor")));
		if (DestBase)
		{
			// TargetActor는 없지만, TargetLocation을 기지로 설정해 이동을 유도
			BB->SetValueAsVector(FName("TargetLocation"), DestBase->GetActorLocation());
		}
	}
}