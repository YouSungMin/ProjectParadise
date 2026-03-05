// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/ProjectilePiercing.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

void AProjectilePiercing::OnPoolActivate_Implementation()
{
	Super::OnPoolActivate_Implementation();

	// 투사체가 새로 발사될 때 타격 명단 초기화
	HitActors.Empty();
}

void AProjectilePiercing::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (!IsValidTarget(OtherActor)) return;

	// 이미 한 번 때린 적이라면 무시 (다단히트 방지)
	if (HitActors.Contains(OtherActor)) return;

	// 새로운 적이라면 명단에 추가하고 데미지 적용
	HitActors.Add(OtherActor);

	if (DamageSpecHandle.IsValid())
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
		if (TargetASC)
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
		}
	}
}
