// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalcHeal.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UExecCalcHeal : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalcHeal();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
