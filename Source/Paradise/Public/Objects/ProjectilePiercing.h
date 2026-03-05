// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects/ProjectileBase.h"
#include "ProjectilePiercing.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API AProjectilePiercing : public AProjectileBase
{
	GENERATED_BODY()

public:
	/** @brief 풀에서 꺼내질 때 이미 맞은 적 명단 초기화를 위해 오버라이드*/
	virtual void OnPoolActivate_Implementation() override;

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	// 이미 타격한 적들을 기억하는 명단 (다단히트 방지)
	UPROPERTY()
	TSet<AActor*> HitActors;
};
