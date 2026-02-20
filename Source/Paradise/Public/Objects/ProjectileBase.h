// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ObjectPoolInterface.h"
#include "GameplayEffectTypes.h"
#include "ProjectileBase.generated.h"


class USphereComponent;
class UStaticMeshComponent;
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

protected:
	// =========================================================================
	// 내부 로직
	// =========================================================================
	/** @brief 적과 겹쳤을 때(Overlap) 데미지를 주고 풀로 돌아가는 함수 */
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** @brief 타이머 만료 또는 충돌 시 스스로 풀로 돌아가는 헬퍼 함수 */
	void ReturnSelfToPool();

protected:
	// =========================================================================
	// 컴포넌트
	// =========================================================================

	/** @brief 충돌 처리를 담당하는 루트 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereComp;

	/** @brief 투사체의 외형 (화살, 파이어볼 등) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	/** @brief 투사체의 이동 로직을 담당 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComp;

	// =========================================================================
	// 설정
	// =========================================================================
	
	/** @brief 투사체의 최대 생존 시간 (초). 이 시간이 지나면 풀로 돌아갑니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float LifeTime = 3.0f;

private:
	/** @brief 배달해야 할 데미지 택배 상자 */
	FGameplayEffectSpecHandle DamageSpecHandle;

	/** @brief 수명 관리용 타이머 핸들 */
	FTimerHandle LifeTimerHandle;
};
