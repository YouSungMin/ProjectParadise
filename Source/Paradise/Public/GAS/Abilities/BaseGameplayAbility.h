// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Interfaces/CombatInterface.h"
#include "BaseGameplayAbility.generated.h"

/**
 * @class UBaseGameplayAbility
 * @brief 프로젝트의 모든 게임플레이 어빌리티(GA)가 상속받아야 하는 기본 클래스입니다.
 * * @details
 * 이 클래스는 반복적인 GAS API 호출을 줄여주는 유틸리티 함수들을 제공합니다.
 * 특히, 캐릭터가 가진 WeaponID를 통해 데이터 관리 서브시스템(Subsystem)에서
 * 실제 무기 데이터를 조회해오는 기능을 포함합니다.
 */

UCLASS()
class PARADISE_API UBaseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	/**
	 * @brief 생성자. 인스턴싱 정책(InstancingPolicy)을 설정합니다.
	 */
	UBaseGameplayAbility();

	// =========================================================================
	// Actor Helpers
	// =========================================================================

	/**
	 * @brief 어빌리티를 실행한 아바타 액터를 ACharacter로 형변환하여 반환합니다.
	 * @return Cast에 성공하면 캐릭터 포인터, 실패하면 nullptr.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Helper")
	ACharacter* GetPlayerCharacterFromActorInfo() const;

	/**
	 * @brief 어빌리티를 실행한 플레이어 컨트롤러를 반환합니다.
	 * @return 컨트롤러 포인터 (AI의 경우 AIController일 수 있음).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|Helper")
	AController* GetPlayerControllerFromActorInfo() const;

	// =========================================================================
	// Effect Helpers
	// =========================================================================

	/**
	 * @brief GameplayEffect를 적용하기 위한 스펙 핸들(SpecHandle)을 생성합니다. (택배 포장)
	 * * @param EffectClass 적용할 GE 클래스 (Blueprint Class).
	 * @param Level 어빌리티 레벨 (기본값: 1.0f). 데미지 계산식 등에서 활용됩니다.
	 * @return 생성된 FGameplayEffectSpecHandle. 실패 시 유효하지 않은 핸들 반환.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Effect")
	FGameplayEffectSpecHandle MakeSpecHandle(TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.0f);

	/** @brief 어빌리티 시작 시 캐릭터에게 현재 사용 중인 스킬 인덱스를 알립니다. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** @brief 어빌리티 종료 시 캐릭터의 스킬 시전 상태를 초기화(-1)합니다. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * @brief 생성된 스펙 핸들을 타겟 액터에게 적용합니다.
	 * * @param TargetActor 이펙트를 맞을 대상 액터.
	 * @param SpecHandle MakeSpecHandle로 생성한 핸들.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Effect")
	void ApplySpecHandleToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& SpecHandle);

	/** @brief 엑셀 데이터의 쿨타임을 적용하기 위해 오버라이드 */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** @brief 코스트(마나)가 충분한지 검사하는 함수 */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** @brief 코스트(마나)를 실제로 깎는 함수 */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
protected:
	/**
	 * @brief 몽타주를 재생하고 종료 콜백(OnMontageCompleted)을 자동으로 연결해주는 헬퍼 함수
	 * @param MontageToPlay 재생할 몽타주
	 * @param TaskInstanceName 태스크 이름 (보통 NAME_None)
	 * @return 생성된 Montage Task (추가적인 바인딩이 필요할 경우 사용)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Animation")
	class UAbilityTask_PlayMontageAndWait* PlayMontageAndWaitCallback(UAnimMontage* MontageToPlay, FName TaskInstanceName = NAME_None);

	/**
	 * @brief 이 어빌리티의 정체성 (평타 vs 스킬)
	 * @details 블루프린트에서 설정합니다. (기본값: BasicAttack)
	 * - GA_Attack_Normal (평타) -> BasicAttack
	 * - GA_Skill_Smash (스킬) -> WeaponSkill
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	ECombatActionType AbilityActionType = ECombatActionType::BasicAttack;

	/**
	 * @brief 캐릭터에게서 전투 데이터를 가져오는 함수 (캐싱 적용됨)
	 * @details "무기가 바뀌지 않는다"는 전제 하에, 최초 1회만 검색하고 이후엔 저장된 값을 씁니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	const FCombatActionData& GetCombatDataFromActor();

	/**
	 * @brief 몽타주 재생이 끝났거나, 중단되었을 때 호출됩니다.
	 * 어빌리티를 종료(EndAbility)시킵니다.
	 */
	UFUNCTION()
	virtual void OnMontageCompleted();
private:
	/** @brief 데이터를 이미 가져왔는지 확인하는 플래그 */
	bool bIsDataCached = false;

	/** @brief 한 번 가져온 데이터를 저장해두는 변수 */
	FCombatActionData CachedCombatData;
};