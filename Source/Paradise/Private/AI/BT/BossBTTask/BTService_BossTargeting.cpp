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
		UE_LOG(LogAI, Error, TEXT("🛑 [보스 타겟팅] 주변(반경 %.1f)에 아무것도 겹치지 않음!"), SearchRadius);
	}

	AActor* BestTarget = nullptr;
	int32 HighestPriority = 999;

	//시야 반경 내의 대상들 순회 (플레이어와 유닛만 검사)
	if (bHit)
	{
		for (const FOverlapResult& Hit : OverlapResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor) continue;
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
			UE_LOG(LogAI, Warning, TEXT("👀 [보스 타겟팅] 탐색된 Pawn: %s"), *HitActor->GetName());

			if (ACharacterBase* HitChar = Cast<ACharacterBase>(HitActor))
			{
				if (HitChar->IsDead()) continue; // 죽은 타겟은 무시
			}

			int32 CurrentPriority = 999;
			bool bIsEnemy = false;

			// 1순위: 플레이어
			if (HitActor->IsA<APlayerBase>())
			{
				bIsEnemy = true;
				CurrentPriority = 1;
			}
			// 2순위: 적 유닛 (퍼밀리어 등)
			else if (AUnitBase* HitUnit = Cast<AUnitBase>(HitActor))
			{
				if (BossUnit->IsEnemy(HitUnit))
				{
					bIsEnemy = true;
					CurrentPriority = 2;
				}
			}

			// 적군일 경우 타겟 갱신
			if (bIsEnemy && CurrentPriority < HighestPriority)
			{
				HighestPriority = CurrentPriority;
				BestTarget = HitActor;

				// 1순위(플레이어)를 찾았다면 주변 탐색 즉시 종료
				if (HighestPriority == 1) break;
			}
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
