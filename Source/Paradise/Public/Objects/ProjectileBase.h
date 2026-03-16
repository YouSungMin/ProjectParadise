// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ObjectPoolInterface.h"
#include "GameplayEffectTypes.h"
#include "Data/Structs/CombatTypes.h"
#include "ProjectileBase.generated.h"


class USphereComponent;
class UNiagaraComponent;
class UProjectileMovementComponent;


UCLASS()
class PARADISE_API AProjectileBase : public AActor, public IObjectPoolInterface
{
	GENERATED_BODY()
	
public:	
	AProjectileBase();

	// =========================================================================
	// Object Pool Interface
	// =========================================================================
	virtual void OnPoolActivate_Implementation() override;
	virtual void OnPoolDeactivate_Implementation() override;

	// =========================================================================
	// GAS 연동
	// =========================================================================

	/** @brief 발사한 어빌리티(RangeBase)로부터 데미지 정보(SpecHandle)를 전달받습니다. */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetDamageSpecHandle(const FGameplayEffectSpecHandle & InSpecHandle);

	/** @brief 투사체의 정보를 적용하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void ApplyCombatData(float InAttackRange, float InAttackRadius, const FProjectileStats& InProjStats);

	/** @brief 내부 데미지 적용 헬퍼 함수 */
	void ApplyDamageToTarget(AActor* TargetActor);

protected:
	// =========================================================================
	// 내부 로직
	// =========================================================================
	/** @brief 적과 겹쳤을 때(Overlap) 데미지를 주고 풀로 돌아가는 함수 */
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** @brief 타이머 만료 또는 충돌 시 스스로 풀로 돌아가는 헬퍼 함수 */
	void ReturnSelfToPool();

	/** @brief 적합한 타겟인지 검사하는 함수 */
	bool IsValidTarget(AActor* OtherActor);
protected:
	// =========================================================================
	// 컴포넌트
	// =========================================================================

	/** @brief 충돌 처리를 담당하는 루트 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereComp;

	/** @brief 투사체의 외형 (나이아가라 이펙트) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> NiagaraComp;

	/** @brief 투사체의 이동 로직을 담당 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComp;

	// =========================================================================
	// 설정
	// =========================================================================
	
	/** @brief 투사체의 최대 생존 시간 (초). 이 시간이 지나면 풀로 돌아갑니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float LifeTime = 3.0f;

	UPROPERTY()
	FProjectileStats CachedProjStats;

	int32 CurrentPierceCount = 0;

	/** @brief 중복 타격 방지용 명단 */
	UPROPERTY()
	TSet<AActor*> HitActors;

	/** @brief 배달해야 할 데미지 택배 상자 */
	FGameplayEffectSpecHandle DamageSpecHandle;

	/** @brief 수명 관리용 타이머 핸들 */
	FTimerHandle LifeTimerHandle;
};
