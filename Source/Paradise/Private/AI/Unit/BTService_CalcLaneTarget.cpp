#include "AI/Unit/BTService_CalcLaneTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/Base/CharacterBase.h"
#include "Objects/HomeBase.h"

UBTService_CalcLaneTarget::UBTService_CalcLaneTarget()
{
	NodeName = TEXT("Calculate Lane Target");
	bNotifyTick = true;
	Interval = 0.2f; // 0.2초 주기로 갱신
}

void UBTService_CalcLaneTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!BBComp || !AICon || !AICon->GetPawn()) return;

	AICon->ClearFocus(EAIFocusPriority::Gameplay);
	FVector MyLoc = AICon->GetPawn()->GetActorLocation();
	float AssignedLaneY = BBComp->GetValueAsFloat(AssignedLaneYKey.SelectedKeyName);

	// 1. MyAIController의 AIPerception이 세팅한 타겟 가져오기
	AActor* CurrentTarget = Cast<AActor>(BBComp->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// 2. 타겟이 없거나, 죽었다면 -> 적 기지로 타겟 변경
	if (!IsValid(CurrentTarget) || (Cast<ACharacterBase>(CurrentTarget) && Cast<ACharacterBase>(CurrentTarget)->IsDead()))
	{
		CurrentTarget = Cast<AActor>(BBComp->GetValueAsObject(EnemyBaseKey.SelectedKeyName));

		// 죽은 타겟이 블랙보드에 남아있다면 비워줌 (AIPerception이 새 적을 찾을 수 있게)
		if (BBComp->GetValueAsObject(TargetActorKey.SelectedKeyName))
		{
			BBComp->ClearValue(TargetActorKey.SelectedKeyName);
		}

	}

	// 3. 목표 좌표 계산 (레인 Y축 고정)
	FVector MoveTarget;
	if (CurrentTarget)
	{
		// [수정된 부분] 타겟이 적 기지(HomeBase)라면 레인을 유지하며 진진
		if (Cast<AHomeBase>(CurrentTarget))
		{
			MoveTarget = FVector(CurrentTarget->GetActorLocation().X, AssignedLaneY, MyLoc.Z);
		}
		else
		{
			// 타겟이 실제 적 유닛(UnitBase)이라면, 사거리가 닿아야 하므로 레인을 무시하고 직접 다가감!
			MoveTarget = CurrentTarget->GetActorLocation();
		}
	}
	else
	{
		MoveTarget = MyLoc;
	}

	BBComp->SetValueAsVector(MoveDestinationKey.SelectedKeyName, MoveTarget);
	APawn* ControlledPawn = AICon->GetPawn();
	float CurrentSpeed = ControlledPawn->GetVelocity().Size();

	if (CurrentSpeed < 10.0f) // 속도가 0에 가까울 때(멈췄을 때)만 출력
	{
		FString TargetStr = CurrentTarget ? CurrentTarget->GetName() : TEXT("없음(NULL)");
		FString Msg = FString::Printf(TEXT("🛑 [%s] 멈춤! | 타겟: %s | 목표좌표: X=%.0f, Y=%.0f"),
			*ControlledPawn->GetName(), *TargetStr, MoveTarget.X, MoveTarget.Y);

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red, Msg);
	}
	
}