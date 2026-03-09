// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTService_BossTargeting.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"

UBTService_BossTargeting::UBTService_BossTargeting()
{
	NodeName = TEXT("Boss Priority Targeting");
	Interval = 1.0f;        // 1초마다 한 번씩 타겟팅 갱신 (성능 최적화)
	RandomDeviation = 0.2f; // 0.8초 ~ 1.2초 사이 랜덤 실행
}

void UBTService_BossTargeting::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// 1. 필요한 컴포넌트 및 폰 가져오기
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	APawn* BossPawn = AIController->GetPawn();
	if (!BossPawn) return;

	UWorld* World = BossPawn->GetWorld();
	if (!World) return;

	// 2. 오버랩 탐색 준비 (보스 주변 SearchRadius 반경)
	FVector BossLocation = BossPawn->GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(BossPawn); // 자기 자신은 탐색에서 제외

	// 💡 ECC_Pawn은 임시 채널입니다. 만약 플레이어/유닛 전용 TraceChannel을 만들었다면 그걸로 변경하세요.
	bool bHit = World->OverlapMultiByChannel(
		OverlapResults,
		BossLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(SearchRadius),
		CollisionParams
	);

	AActor* BestTarget = nullptr;
	int32 HighestPriority = 999; // 숫자가 작을수록 높은 우선순위

	// 3. 탐색된 대상들을 순회하며 우선순위 비교
	if (bHit)
	{
		for (const FOverlapResult& Hit : OverlapResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor) continue;

			if (ACharacterBase* HitChar = Cast<ACharacterBase>(HitActor))
			{
				if (HitChar->IsDead())
				{
					continue;
				}
			}

			int32 CurrentPriority = 999;

			// 우선순위 판별 (1: 플레이어, 2: 유닛, 3: 본진)
			if (HitActor->IsA<APlayerBase>())
			{
				CurrentPriority = 1;
			}
			else if (HitActor->IsA<AUnitBase>())
			{
				CurrentPriority = 2;
			}
			else if (HitActor->IsA<AHomeBase>())
			{
				CurrentPriority = 3;
			}

			// 현재 저장된 타겟보다 우선순위가 높다면 교체
			if (CurrentPriority < HighestPriority)
			{
				HighestPriority = CurrentPriority;
				BestTarget = HitActor;

				// 만약 1순위(플레이어)를 찾았다면 더 이상 찾을 필요가 없으므로 루프 즉시 종료 (최적화)
				if (HighestPriority == 1)
				{
					break;
				}
			}
		}
	}

	// 4. 최종 결정된 타겟을 블랙보드에 기록
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, BestTarget);
	}
}
