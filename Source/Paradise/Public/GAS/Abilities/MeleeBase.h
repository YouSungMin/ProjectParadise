// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/BaseGameplayAbility.h"
#include "MeleeBase.generated.h"

/**
 * @class UGA_MeleeBase
 * @brief 근접 공격(Melee Attack)을 위한 공용 로직 클래스입니다.
 * * @details
 * 이 어빌리티는 스스로 데이터를 가지고 있지 않습니다.
 * Base 클래스의 GetEquippedWeaponAssets()를 통해 현재 장착된 무기의
 * '공격 몽타주'와 '데미지 이펙트'를 가져와서 실행합니다.
 */
UCLASS()
class PARADISE_API UMeleeBase : public UBaseGameplayAbility
{
	GENERATED_BODY()

public:
	UMeleeBase();

	/**
	 * @brief 어빌리티 실행 진입점.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
protected:

	// =========================================================================
	// Configuration (설정)
	// =========================================================================

	/**
	 * @brief 타격 판정을 감지할 이벤트 태그입니다.
	 * 기본값: "Event.Montage.Hit"
	 * 애니메이션 노티파이(SendGameplayEvent)에서 보내는 태그와 일치해야 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FGameplayTag HitEventTag;

	// =========================================================================
	// Callbacks (내부 로직)
	// =========================================================================

	/**
	 * @brief WaitGameplayEvent 태스크에서 이벤트(타격)가 감지되었을 때 호출됩니다.
	 * @param Payload 이벤트 데이터 (여기에 맞은 적 Target이 들어있음).
	 */
	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);
};
