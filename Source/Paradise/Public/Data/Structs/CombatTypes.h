#pragma once

#include "CoreMinimal.h"
#include "Data/Enums/GameEnums.h"
#include "CombatTypes.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UGameplayAbility;

/**
 * @struct FCombatAbilitySetup
 * @brief 하나의 전투 행동(평타, 스킬 등)에 필요한 필수 GAS 로직 세트
 * @details 어빌리티(로직), 이펙트(결과), 투사체(수단)를 하나로 묶어 관리합니다.
 */
USTRUCT(BlueprintType)
struct FCombatAbilitySetup
{
	GENERATED_BODY()

public:
	/** @brief 실행할 게임플레이 어빌리티 (GA) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Logic")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** @brief 적중 시 적에게 적용할 데미지/상태이상 이펙트 (GE) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Logic")
	TSubclassOf<UGameplayEffect> EffectClass;

	/** * @brief 발사할 투사체 액터 클래스
	 * @note 근거리 공격이나 즉발형 스킬일 경우 비워둡니다 (None).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Logic")
	TSubclassOf<AActor> ProjectileClass;
};


/**
 * @struct FActionStatRow
 * @brief 엑셀(데이터 테이블)에서 개별 액션(평타1, 스킬1, 몬스터공격 등)의 수치를 정의하는 구조체
 */
USTRUCT(BlueprintType)
struct FActionStats : public FTableRowBase
{
	GENERATED_BODY()

	/** @brief 데미지 배율 (기본 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	float DamageMultiplier = 1.0f;

	/**
	 * @brief 공격 사거리 (Attack Range)
	 * @details 단위: cm (Unreal Unit).
	 * 근거리 무기의 경우 충돌(Trace) 검사 길이로 사용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	float AttackRange = 100.0f;

	/** @brief 공격 반경 (두께/넓이) - 도약 공격 등 광역 판정용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	float AttackRadius = 40.0f;

	/**
	 * @brief 전방 오프셋 (캐릭터 앞쪽으로 얼마나 밀어서 타격할 것인가)
	 * @details 단위: cm (Unreal Unit)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	float ForwardOffset = 50.0f;

	/**
	 * @brief 스킬 재사용 대기시간 (Cooldown)
	 * @details 단위: 초 (Seconds).
	 * GAS의 Cooldown GameplayEffect(GE_Cooldown)에 적용될 지속 시간(Duration)입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats", meta = (ClampMin = "0.0"))
	float Cooldown;

	/** @brief 스킬 사용 시 소모되는 마나량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats", meta = (ClampMin = "0.0"))
	float ManaCost = 0.0f;

	/** @brief 스킬의 적용 대상 (적군인지 아군인지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	ETargetFilter TargetFilter = ETargetFilter::Enemy;

	/** @brief 버프/디버프 지속 시간 (0이면 즉발형) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	float BuffDuration = 0.0f;

	/** @brief 애니메이션 기본 재생 속도 배율, 기본값은 1.0입니다.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	float AnimPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Extra")
	FDataTableRowHandle ProjectileDataHandle;
};

/**
 * @struct FProjectileStats
 * @brief 원거리 스킬(투사체)에만 사용되는 전용 전투 수치 데이터 (별도의 엑셀 테이블로 관리)
 */
USTRUCT(BlueprintType)
struct FProjectileStats : public FTableRowBase
{
	GENERATED_BODY()

	/** @brief 한 번에 발사할 투사체 개수 (기본 1발) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	int32 ProjectileCount = 1;

	/** @brief 다중 발사 시 퍼지는 총 각도 (예: 부채꼴 45도) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float SpreadAngle = 0.0f;

	/** @brief 투사체 비행 속도 (기존 메인 테이블에서 이사 옴!) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float ProjectileSpeed = 1500.0f;

	// 유도탄 여부, 폭발 반경, 중력 값 등 투사체에만 필요한 설정 추가 가능
};

USTRUCT(BlueprintType)
struct FCombatActionData
{
	GENERATED_BODY()

public:
	FCombatActionData()
		: MontageToPlay(nullptr)
		, EffectClass(nullptr)
		, ProjectileClass(nullptr)
	{
	}

	/* @brief 재생할 공격 몽타주
	 * @details 플레이어: 무기 에셋의 AttackMontage / 몬스터: 본인의 AttackMontage
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> MontageToPlay;

	/* @brief 적용할 데미지 GE 클래스
	 * @details 데미지 계산 공식(ExecutionCalculation)이 연결된 GE입니다.
	 * (예: GE_DamageStandard, GE_FireDamage)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	TSubclassOf<UGameplayEffect> EffectClass;

	// =====================================
	//  원거리 전용 (Ranged)
	// =====================================
	/* @brief 발사할 투사체 클래스
	 * @details 이 값이 비어있으면(None) '근거리(Melee)'로 간주하고, 값이 있으면 '원거리(Ranged)'로 간주합니다.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Ranged")
	TSubclassOf<AActor> ProjectileClass;

	// =====================================
	// 전투 수치 데이터
	// =====================================

	/** @brief 엑셀(데이터 테이블)에서 개별 액션(평타1, 스킬1, 몬스터공격 등)의 수치를 정의하는 구조체*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Stats")
	FActionStats Stats;

	// ==========================================================
	// 투사체 전용 데이터 캐싱 공간
	// ==========================================================
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Stats")
	FProjectileStats ProjectileStats;

	/** @brief 이 액션이 투사체 데이터를 가지고 있는지(원거리인지) 확인하는 헬퍼 함수 */
	bool HasProjectileStats() const
	{
		return !Stats.ProjectileDataHandle.IsNull();
	}
};


