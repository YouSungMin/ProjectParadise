// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CheckStuck.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTService_CheckStuck : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_CheckStuck();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Stuck")
	struct FBlackboardKeySelector TargetActorKey;

	// 이 거리(cm) 미만으로 움직였다면 막힌 것으로 간주
	UPROPERTY(EditAnywhere, Category = "Stuck")
	float StuckDistanceThreshold = 15.0f;

	// 몇 초 동안 막혀있어야 타겟을 포기할 것인가?
	UPROPERTY(EditAnywhere, Category = "Stuck")
	float StuckTimeLimit = 2.0f;

private:
	// AI 개별 인스턴스마다 기억할 변수들
	FVector LastLocation;
	float StuckTime;
	
};
