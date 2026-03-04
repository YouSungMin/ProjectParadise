// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/BaseGameplayAbility.h"
#include "ProjectileAttackBase.generated.h"

/**
 * @class URangeBase
 * @brief 원거리 공격(Ranged Attack)을 위한 공용 로직 클래스입니다.
 * @details 몽타주를 재생하고 이벤트가 발생하면 설정된 ProjectileClass를 스폰합니다.
 */
UCLASS()
class PARADISE_API UProjectileAttackBase : public UBaseGameplayAbility
{
	GENERATED_BODY()
public:
	UProjectileAttackBase();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/**
	 * @brief 발사 시점을 감지할 이벤트 태그입니다.
	 * 기본값: "Event.Montage.Fire" (애니메이션 노티파이에서 이 태그를 보내야 합니다)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FGameplayTag FireEventTag;

	/**
	 * @brief 투사체가 발사될 소켓의 이름입니다.
	 * 캐릭터 메쉬나 무기 메쉬에 이 이름의 소켓이 있어야 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FName MuzzleSocketName = FName("MuzzleSocket");

	// =========================================================================
	// Callbacks
	// =========================================================================

	/**
	 * @brief WaitGameplayEvent 태스크에서 발사 이벤트가 감지되었을 때 호출됩니다.
	 */
	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);
};
