// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FindClosestTarget.generated.h"

UCLASS()
class PARADISE_API UBTService_FindClosestTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_FindClosestTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "AI")
	float SearchRadius = 3000.0f;
};