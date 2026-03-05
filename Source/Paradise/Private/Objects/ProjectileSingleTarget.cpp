// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/ProjectileSingleTarget.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

void AProjectileSingleTarget::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (!IsValidTarget(OtherActor)) return;

	// 데미지 적용
	if (DamageSpecHandle.IsValid())
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
		if (TargetASC)
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
		}
	}

	// 단일 타겟이므로 한 대 치면 바로 풀로 반환
	ReturnSelfToPool();
}
