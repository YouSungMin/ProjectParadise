// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/AIUnit/UnitBase.h"
#include "HomeBase.generated.h"

UCLASS()
class PARADISE_API AHomeBase : public AUnitBase
{
	GENERATED_BODY()

public:
	AHomeBase();

protected:
	virtual void BeginPlay() override;

	virtual void Die() override;
};