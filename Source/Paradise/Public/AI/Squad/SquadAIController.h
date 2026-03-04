// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "SquadAIController.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API ASquadAIController : public AAIController
{
	GENERATED_BODY()
	

public:
	ASquadAIController();

	/** @brief 인게임 컨트롤러에서 캐릭터 스위치 시 호출해 줄 리더 지정 함수 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetLeader(AActor* CurrentLeaderActor);

protected:

	virtual void OnPossess(APawn* InPawn) override;

private:
	// AI 에셋
	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<class UBehaviorTree> BTAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<class UBlackboardData> BBAsset=nullptr;

	// 시야 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<class UAIPerceptionComponent> AIPerception = nullptr;

	UPROPERTY()
	TObjectPtr<class UAISenseConfig_Sight> SightConfig = nullptr;

	// 타겟 감지 이벤트
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);
};
