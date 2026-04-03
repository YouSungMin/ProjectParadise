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

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	virtual void OnMontageCompleted() override;

protected:
	/**
	 * @brief 발사 시점을 감지할 이벤트 태그입니다.
	 * 기본값: "Event.Montage.Fire" (애니메이션 노티파이에서 이 태그를 보내야 합니다)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FGameplayTag FireEventTag;

	// =========================================================================
	// Callbacks
	// =========================================================================

	/** @brief WaitGameplayEvent 태스크에서 발사 이벤트가 감지되었을 때 호출됩니다 */
	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);

	/** @brief 투사체 1발을 실제로 스폰하는 헬퍼 함수 (타이머가 반복 호출함) */
	UFUNCTION()
	void FireSinglePellet();

protected:
	/** @brief 현재 몇 발째 발사 중인지 카운트 */
	int32 BurstCurrentCount = 0;

	/** @brief 총구 위치 캐싱 */
	FTransform CachedMuzzleTransform;

	/** @brief 연사(타이머)가 진행 중인지 체크 */
	bool bIsShooting = false;

	/** @brief 애니메이션 몽타주가 끝났는지 체크 */
	bool bMontageFinished = false;
};
