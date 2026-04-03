// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CheckLeaderDistance.generated.h"

/**
 * 
 */
UCLASS()	
class PARADISE_API UBTService_CheckLeaderDistance : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_CheckLeaderDistance();

protected:
	/** 리더와의 거리를 검사하는 함수 */
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 리더와의 최대 거리 (넘어가면 돌아옴) */
	UPROPERTY(EditAnywhere, Category = "AI")
	float MaxChaseDistance = 1200.0f;
};
