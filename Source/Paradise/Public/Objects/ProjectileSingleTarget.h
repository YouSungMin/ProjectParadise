// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects/ProjectileBase.h"
#include "ProjectileSingleTarget.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API AProjectileSingleTarget : public AProjectileBase
{
	GENERATED_BODY()
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
};
