// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_FindClosestTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"

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

	UObject* RawTarget = BB->GetValueAsObject(TargetActorKey.SelectedKeyName);
	ACharacterBase* CurrentTarget = Cast<ACharacterBase>(RawTarget);

	ACharacterBase* ClosestEnemy = nullptr;
	float MinDistance = SearchRadius;

	// [1단계] 현재 타겟의 거리를 측정하여 비교의 '기준점'으로 삼습니다. (바로 return 하지 않음!)
	if (IsValid(CurrentTarget) && !CurrentTarget->IsDead())
	{
		float CenterDist = FVector::Dist(SelfUnit->GetActorLocation(), CurrentTarget->GetActorLocation());
		float MyRadius = SelfUnit->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float TargetRadius = CurrentTarget->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float SurfaceDist = FMath::Max(0.0f, CenterDist - MyRadius - TargetRadius);

		if (SurfaceDist < SearchRadius)
		{
			MinDistance = SurfaceDist;
			ClosestEnemy = CurrentTarget; // 일단 현재 타겟을 가장 가까운 적으로 임시 지정
		}
	}

	// [2단계] 월드 내 모든 적을 탐색하며 '더 가까운 적'이 있는지 검사합니다.
	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		ACharacterBase* OtherChar = *It;

		// 무시할 대상들
		if (OtherChar->IsA<AHomeBase>()) continue;
		if (!IsValid(OtherChar) || OtherChar == SelfUnit || OtherChar->IsDead()) continue;
		if (OtherChar == ClosestEnemy) continue; // 이미 1단계에서 계산한 현재 타겟은 건너뜀

		if (SelfUnit->IsHostile(OtherChar))
		{
			float CenterToCenterDistance = FVector::Dist(SelfUnit->GetActorLocation(), OtherChar->GetActorLocation());
			float MyRadius = SelfUnit->GetCapsuleComponent()->GetScaledCapsuleRadius();
			float TargetRadius = OtherChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
			float SurfaceDistance = FMath::Max(0.0f, CenterToCenterDistance - MyRadius - TargetRadius);

			// 🚨 [핵심] 핑퐁(Ping-Pong) 방지 로직
			// 타겟이 없는 상태였다면 그냥 갱신하고, 기존 타겟이 있었다면 기존 타겟보다 '50 유닛' 이상 더 가까워야만 타겟을 바꿉니다.
			float SwitchThreshold = (ClosestEnemy == CurrentTarget && CurrentTarget != nullptr) ? (MinDistance - 50.0f) : MinDistance;

			if (SurfaceDistance < SwitchThreshold)
			{
				MinDistance = SurfaceDistance;
				ClosestEnemy = OtherChar; // 더 가까운 적으로 갱신!
			}
		}
	}

	// [3단계] 탐색 결과 반영 (타겟이 유지되었거나 더 가까운 놈으로 변경됨)
	if (ClosestEnemy)
	{
		// 타겟이 바뀌었는지 체크 (디버그 로그)
		if (CurrentTarget != nullptr && CurrentTarget != ClosestEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("🔄 [타겟 변경] 나(%s) : 더 가까운 적(%s, 거리: %.1f)을 발견하여 타겟을 변경합니다!"),
				*SelfUnit->GetName(), *ClosestEnemy->GetName(), MinDistance);
		}

		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, ClosestEnemy);
		BB->SetValueAsFloat(FName("DistanceToTarget"), MinDistance);

		float MyAttackRange = SelfUnit->GetAttackRange();

		if (MinDistance <= MyAttackRange)
		{
			// 🚨 [여기에 추가 1] 사거리 진입 감시 로그 추가!
			UE_LOG(LogTemp, Error, TEXT("🟢 [서비스] %s ➔ %s : 사거리 진입 성공! (거리: %.1f / 사거리: %.1f) ➔ bIsInRange = TRUE 설정!"),
				*SelfUnit->GetName(), *ClosestEnemy->GetName(), MinDistance, MyAttackRange);

			BB->SetValueAsBool(FName("bIsInRange"), true);
			BB->SetValueAsVector(FName("TargetLocation"), SelfUnit->GetActorLocation()); // 제자리 정지
		}
		else
		{
			// 🚨 [여기에 추가 2] 사거리 밖 감시 로그 추가!
			UE_LOG(LogTemp, Warning, TEXT("🔴 [서비스] %s ➔ %s : 아직 멉니다.. (거리: %.1f / 사거리: %.1f) ➔ bIsInRange = FALSE 설정!"),
				*SelfUnit->GetName(), *ClosestEnemy->GetName(), MinDistance, MyAttackRange);

			BB->SetValueAsBool(FName("bIsInRange"), false);

			// 뭉침 방지 분산 로직 (Surround Logic)
			FVector RandomOffset = FMath::VRand();
			RandomOffset.Z = 0.0f;
			RandomOffset.Normalize();

			FVector SpreadLocation = ClosestEnemy->GetActorLocation() + (RandomOffset * FMath::RandRange(50.0f, 100.0f));
			BB->SetValueAsVector(FName("TargetLocation"), SpreadLocation);
		}
	}
	else
	{
		// [4단계] 주변에 적 유닛이 없는 경우 (기지 이동 로직)
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
		BB->ClearValue(FName("DistanceToTarget"));

		// 🚨 [여기에 추가 3] 적이 아예 없으므로 공격 불가(false) 처리 (안전장치)
		BB->SetValueAsBool(FName("bIsInRange"), false);

		FVector CurrentTargetLoc = BB->GetValueAsVector(FName("TargetLocation"));
		AActor* DestBase = Cast<AActor>(BB->GetValueAsObject(FName("EnemyBaseActor")));

		if (DestBase && CurrentTargetLoc != FVector::ZeroVector)
		{
			float DistToBase = FVector::Dist(CurrentTargetLoc, DestBase->GetActorLocation());
			if (DistToBase < 1000.0f)
			{
				return;
			}
		}

		if (DestBase)
		{
			FVector BaseLocation = DestBase->GetActorLocation();
			FVector RandomOffset = FMath::VRand();
			RandomOffset.Z = 0.0f;
			RandomOffset.Normalize();

			float RandomRadius = FMath::RandRange(300.0f, 700.0f);
			FVector FinalTargetLocation = BaseLocation + (RandomOffset * RandomRadius);

			BB->SetValueAsVector(FName("TargetLocation"), FinalTargetLocation);
		}
	}
}