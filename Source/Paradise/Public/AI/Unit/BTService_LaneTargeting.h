// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_LaneTargeting.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTService_LaneTargeting : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_LaneTargeting();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 타겟을 저장할 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetActorKey;

	// 적 기지를 저장해둘 블랙보드 키 (스포너에서 미리 넣어두면 최적화에 좋습니다)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector EnemyBaseKey;

	// 탐색 반경
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float SearchRadius = 800.0f;

	// 적군 콜리전 오브젝트 타입 (Project Settings에서 설정한 EnemyUnit 채널)
	UPROPERTY(EditAnywhere, Category = "Targeting")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;
	
};
