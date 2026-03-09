#pragma once

#include "CoreMinimal.h"
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

	/** * @brief 투사체 비행 속도
	 * @details 이 값이 비어있으면(None) '근거리(Melee), 값이 있으면 '원거리(Ranged)'로 간주합니다.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Stats")
	float ProjectileSpeed;
};

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
	//  타격 판정 공용 스탯 (Hit Check)
	// =====================================

	/** * @brief 공격 사거리 (길이/Reach)
	 * @details
	 * - 근거리: 전방으로 무기가 닿는 최대 거리 (검=100, 창=300)
	 * - 원거리: AI의 타겟팅 인식 거리 또는 투사체의 최대 비행 거리
	 * - 제자리 광역기(Slam): 0으로 설정
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|HitCheck")
	float AttackRange = 150.0f;

	/** * @brief 공격 반경 (두께/넓이)
	 * @details
	 * - 근거리: 무기의 두께 (보통 20~50)
	 * - 제자리 광역기: 폭발 반경 (보통 300~500)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|HitCheck")
	float AttackRadius = 40.0f;

	/** * @brief 전방 오프셋 (판정 시작점)
	 * @details 캐릭터 중심에서 앞쪽으로 얼마나 떨어진 곳에서 타격을 시작/폭발시킬 것인가
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|HitCheck")
	float ForwardOffset = 0.0f;

	/** @brief 엑셀에서 읽어온 스킬 쿨타임 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Stats")
	float Cooldown = 0.0f;

	/** @brief 엑셀에서 읽어온 소모되는 마나량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats", meta = (ClampMin = "0.0"))
	float ManaCost = 0.0f;

	// =====================================
	//  원거리 전용 (Ranged)
	// =====================================
	/** * @brief 발사할 투사체 클래스
	 * @details 이 값이 비어있으면(None) '근거리(Melee)'로 간주하고, 값이 있으면 '원거리(Ranged)'로 간주합니다.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Ranged")
	TSubclassOf<AActor> ProjectileClass;

	/** * @brief 투사체 비행 속도 
	 * @details 이 값이 비어있으면(None) '근거리(Melee), 값이 있으면 '원거리(Ranged)'로 간주합니다.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Stats")
	float ProjectileSpeed = 0.0f;
};


