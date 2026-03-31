// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "DetourCrowdAIController.h"
#include "MyAIController.generated.h"

UCLASS()
class PARADISE_API AMyAIController : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	AMyAIController();

	/** @brief 데이터 테이블에서 스탯을 읽어 블랙보드에 기록하는 함수 */
	void LoadUnitStatsFromTable();

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY(EditAnywhere, Category = "AI")
	class UBehaviorTree* BTAsset;

	UPROPERTY(EditAnywhere, Category = "AI")
	class UBlackboardData* BBAsset;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	class UAIPerceptionComponent* AIPerception;

	UPROPERTY()
	class UAISenseConfig_Sight* SightConfig;

	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);
};