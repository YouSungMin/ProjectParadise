// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTService_BossTargeting.h"
#include "Paradise/Paradise.h" //로그
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "Kismet/GameplayStatics.h"
UBTService_BossTargeting::UBTService_BossTargeting()
{
	NodeName = TEXT("Boss Priority Targeting");
	Interval = 1.0f;        // 1초마다 한 번씩 타겟팅 갱신 (성능 최적화)
	RandomDeviation = 0.2f; // 0.8초 ~ 1.2초 사이 랜덤 실행
}

void UBTService_BossTargeting::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	APawn* BossPawn = AIController->GetPawn();
	if (!BossPawn) return;

	AUnitBase* BossUnit = Cast<AUnitBase>(BossPawn);
	if (!BossUnit) return;

	UWorld* World = BossPawn->GetWorld();
	if (!World) return;

	//오버랩 탐색 준비
	FVector BossLocation = BossPawn->GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(BossPawn); // 자기 자신 무시

	//채널 검사가 아니라, 'Pawn' 오브젝트 타입만 찝어서 검사합니다!
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	bool bHit = World->OverlapMultiByObjectType(
		OverlapResults,
		BossLocation,
		FQuat::Identity,
		ObjectQueryParams, // 변경된 부분
		FCollisionShape::MakeSphere(SearchRadius),
		CollisionParams
	);

	if (!bHit)
	{
		UE_LOG(LogParadiseAI, Error, TEXT("🛑 [보스 타겟팅] 주변(반경 %.1f)에 아무것도 겹치지 않음!"), SearchRadius);
	}

	AActor* BestTarget = nullptr;
	int32 HighestPriority = 999;
	float ClosestDistance = MAX_flt;
	//시야 반경 내의 대상들 순회 (플레이어와 유닛만 검사)
	if (bHit)
	{
		for (const FOverlapResult& Hit : OverlapResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor) continue;

			if (ACharacterBase* HitChar = Cast<ACharacterBase>(HitActor))
			{
				if (HitChar->IsDead()) continue;
			}

			int32 CurrentPriority = 999;
			bool bIsEnemy = false;

			// 1순위: 플레이어, 2순위: 적 유닛 판별 로직 (기존과 동일)
			if (HitActor->IsA<APlayerBase>())
			{
				bIsEnemy = true;
				CurrentPriority = 1;
			}
			else if (AUnitBase* HitUnit = Cast<AUnitBase>(HitActor))
			{
				if (BossUnit->IsEnemy(HitUnit))
				{
					bIsEnemy = true;
					CurrentPriority = 2;
				}
			}

			if (bIsEnemy)
			{
				// 🚨 [추가] 보스와 이 타겟 사이의 실제 거리를 계산합니다.
				float DistanceToTarget = FVector::DistSquared(BossLocation, HitActor->GetActorLocation());

				// 경우 1: 기존 타겟보다 "우선순위가 더 높은(숫자가 작은) 적"을 발견했을 때 (예: 유닛 치려다가 플레이어 발견)
				if (CurrentPriority < HighestPriority)
				{
					HighestPriority = CurrentPriority;
					ClosestDistance = DistanceToTarget; // 거리 갱신
					BestTarget = HitActor;
				}
				// 경우 2: "같은 우선순위"의 적을 발견했을 때 (예: 플레이어를 발견했는데, 또 다른 플레이어 발견)
				else if (CurrentPriority == HighestPriority)
				{
					// 누가 더 가까운지 비교해서, 더 가까운 놈으로 타겟을 갈아치웁니다!
					if (DistanceToTarget < ClosestDistance)
					{
						ClosestDistance = DistanceToTarget; // 더 짧은 거리로 갱신
						BestTarget = HitActor;
					}
				}
			}

			// 🚨 [삭제] 기존에 있던 'if (HighestPriority == 1) break;' 구문은 반드시 지워주세요! 
			// 모든 적을 끝까지 비교해야 가장 가까운 놈을 찾을 수 있습니다.
		}
	}
	// 거리 상관없이 맵 전체에서 '적 기지'를 찾아서 기본 타겟으로 설정
	if (!BestTarget)
	{
		TArray<AActor*> FoundBases;
		UGameplayStatics::GetAllActorsOfClass(World, AHomeBase::StaticClass(), FoundBases);

		for (AActor* BaseActor : FoundBases)
		{
			AHomeBase* HomeBase = Cast<AHomeBase>(BaseActor);
			// 기지의 팩션 태그와 내 태그가 다르면 적 기지!
			if (HomeBase && !BossUnit->GetFactionTag().MatchesTag(HomeBase->GetFactionTag()))
			{
				BestTarget = HomeBase;
				break; // 적 기지를 찾았으므로 루프 종료
			}
		}
	}

	//최종 결정된 타겟(적, 혹은 적 기지)을 블랙보드에 기록
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsObject(TEXT("TargetActor"), BestTarget);
	}
}
