// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Unit/BTService_CheckStuck.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Objects/HomeBase.h"
#include "Engine/Engine.h"

UBTService_CheckStuck::UBTService_CheckStuck()
{
	NodeName = TEXT("Check Stuck");
	bNotifyTick = true;

	// 💡 중요: AI가 여러 마리일 때 각자 독립적인 타이머와 위치를 기억하도록 인스턴스를 분리합니다.
	bCreateNodeInstance = true;
	Interval = 0.5f; // 0.5초마다 검사
}

void UBTService_CheckStuck::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();

	if (!AICon || !AICon->GetPawn() || !BBComp) return;

	APawn* ControlledPawn = AICon->GetPawn();
	FVector CurrentLocation = ControlledPawn->GetActorLocation();
	AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// 타겟이 존재하고, 그 타겟이 기지가 아닐 때만 길막힘 체크 (기지 때릴 땐 비벼도 됨)
	if (TargetActor && !Cast<AHomeBase>(TargetActor))
	{
		// XY 평면 이동 거리만 체크 (언덕/단차 오를 때 Z값 튀는 것 방지)
		float DistanceMoved = FVector::Dist2D(CurrentLocation, LastLocation);

		if (DistanceMoved < StuckDistanceThreshold)
		{
			// 거의 못 움직임 -> 타이머 증가
			StuckTime += Interval;

			if (StuckTime >= StuckTimeLimit)
			{
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString::Printf(TEXT("⚠️ [%s] 꽉 막힘 감지! 타겟(%s) 포기!"), *ControlledPawn->GetName(), *TargetActor->GetName()));

				// 타겟 비우기 (다른 적을 찾거나 전진하도록)
				BBComp->ClearValue(TargetActorKey.SelectedKeyName);
				StuckTime = 0.0f;
			}
		}
		else
		{
			// 잘 움직이고 있다면 타이머 초기화
			StuckTime = 0.0f;
		}
	}
	else
	{
		StuckTime = 0.0f;
	}

	LastLocation = CurrentLocation;
}
