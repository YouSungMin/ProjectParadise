#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

class UAnimMontage;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FCombatActionData
{
	GENERATED_BODY()

public:
	FCombatActionData()
		: MontageToPlay(nullptr)
		, DamageEffectClass(nullptr)
		, DamageMultiplier(1.0f)
		, ProjectileClass(nullptr)
		, AttackRange(150.0f)
	{
	}

	/** * @brief 재생할 공격 몽타주
	 * @details 플레이어: 무기 에셋의 AttackMontage / 몬스터: 본인의 AttackMontage
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** * @brief 적용할 데미지 GE 클래스
	 * @details 데미지 계산 공식(ExecutionCalculation)이 연결된 GE입니다.
	 * (예: GE_DamageStandard, GE_FireDamage)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** * @brief 데미지 계수 (Damage Multiplier)
	 * @details 기본값 1.0.
	 * 평타는 1.0, 스킬은 1.5, 궁극기는 3.0 등으로 설정하여 데미지 공식을 조절합니다.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float DamageMultiplier;

	// =====================================
	//  원거리 전용 (Ranged)
	// =====================================
	/** * @brief 발사할 투사체 클래스
	 * @details 이 값이 비어있으면(None) '근거리(Melee)'로 간주하고, 값이 있으면 '원거리(Ranged)'로 간주합니다.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Ranged")
	TSubclassOf<AActor> ProjectileClass;

	/** * @brief 공격 사거리
	 * @details AI가 공격을 시작할 거리 또는 투사체의 사거리 등을 결정합니다.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Ranged")
	float AttackRange;
};
