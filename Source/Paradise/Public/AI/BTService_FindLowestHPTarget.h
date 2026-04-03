// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FindLowestHPTarget.generated.h"

UCLASS()
class PARADISE_API UBTService_FindLowestHPTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_FindLowestHPTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "AI")
	float SearchRadius;
};