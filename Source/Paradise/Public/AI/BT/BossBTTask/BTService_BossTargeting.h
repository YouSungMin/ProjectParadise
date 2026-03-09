// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_BossTargeting.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTService_BossTargeting : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_BossTargeting();

protected:
	// 매 틱(Interval)마다 실행될 핵심 탐색 로직
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:

	/** @brief 보스의 탐색 반경 */
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float SearchRadius = 3000.0f; 

	/** @brief 탐색된 타겟을 저장할 블랙보드 변수 지정 */
	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetKey; 
};
