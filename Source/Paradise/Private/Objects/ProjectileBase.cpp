// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Characters/Base/CharacterBase.h"

// Sets default values
AProjectileBase::AProjectileBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SetRootComponent(SphereComp);
	SphereComp->InitSphereRadius(15.0f);
	SphereComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &AProjectileBase::OnSphereOverlap);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(SphereComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
	ProjectileMovementComp->InitialSpeed = 1000.0f;
	ProjectileMovementComp->MaxSpeed = 2000.0f;
	ProjectileMovementComp->ProjectileGravityScale = 0.0f;
	// 풀링을 위해 자동 활성화를 끕니다. (OnPoolActivate에서 켭니다)
	ProjectileMovementComp->bAutoActivate = false;
}

void AProjectileBase::OnPoolActivate_Implementation()
{
	SetActorHiddenInGame(false);

	// 이동 재시작
	ProjectileMovementComp->SetComponentTickEnabled(true);
	ProjectileMovementComp->Activate(true);
	ProjectileMovementComp->Velocity = GetActorForwardVector() * ProjectileMovementComp->InitialSpeed;

	// LifeTime 이후에 풀로 돌아가는 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(LifeTimerHandle, this, &AProjectileBase::ReturnSelfToPool, LifeTime, false);
}

void AProjectileBase::OnPoolDeactivate_Implementation()
{
	// 타이머 초기화
	GetWorld()->GetTimerManager().ClearTimer(LifeTimerHandle);

	// 물리/이동 정지 및 숨김
	ProjectileMovementComp->Deactivate();
	ProjectileMovementComp->SetComponentTickEnabled(false);
	ProjectileMovementComp->Velocity = FVector::ZeroVector;

	SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);

	// 갖고 있던 데미지 정보(택배) 파기
	DamageSpecHandle = FGameplayEffectSpecHandle();
}

void AProjectileBase::SetDamageSpecHandle(const FGameplayEffectSpecHandle& InSpecHandle)
{
	DamageSpecHandle = InSpecHandle;
}

void AProjectileBase::ApplyCombatData(float InAttackRange, float InAttackRadius, float InSpeed)
{
	// 투사체 판정 크기(두께) 변경
	if (SphereComp)
	{
		SphereComp->SetSphereRadius(InAttackRadius);
	}

	// 시각적 메쉬 크기 조절
	float ScaleRatio = InAttackRadius / 15.0f;
	SetActorScale3D(FVector(ScaleRatio));

	if (InSpeed > 0.0f)
	{
		ProjectileMovementComp->InitialSpeed = InSpeed;
		ProjectileMovementComp->MaxSpeed = InSpeed;

		// [중요] 풀링에서 꺼내진 상태이므로 Velocity를 직접 갱신해 줘야 바로 적용됩니다!
		ProjectileMovementComp->Velocity = GetActorForwardVector() * InSpeed;
	}

	// 사거리(거리)를 기반으로 생존 시간(LifeTime) 계산 (시간 = 거리 / 속력)
	float CurrentSpeed = ProjectileMovementComp->InitialSpeed;
	if (CurrentSpeed > 0.0f)
	{
		LifeTime = InAttackRange / CurrentSpeed;
	}
	UE_LOG(LogTemp, Warning, TEXT("⏱️ [Projectile] 사거리: %.1f -> 계산된 수명: %.3f초"), InAttackRange, LifeTime);
	// 타이머 재설정
	// 이미 OnPoolActivate에서 기본 LifeTime으로 타이머가 돌고 있으므로,
	// 기존 타이머를 취소하고 계산된 정확한 생존 시간으로 다시 타이머를 가동합니다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifeTimerHandle);
		World->GetTimerManager().SetTimer(LifeTimerHandle, this, &AProjectileBase::ReturnSelfToPool, LifeTime, false);
	}

	if (SphereComp)
	{
		SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void AProjectileBase::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 자기 자신이나, 나를 쏜 주인(Instigator)은 무시
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator()) return;

	if (ACharacterBase* Shooter = Cast<ACharacterBase>(GetInstigator()))
	{
		if (ACharacterBase* TargetChar = Cast<ACharacterBase>(OtherActor))
		{
			if (!Shooter->IsHostile(TargetChar))
			{
				return;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("🎯 [Projectile] 투사체 적중! 대상: %s"), *OtherActor->GetName());

	if (DamageSpecHandle.IsValid())
	{
		// Target의 ASC를 찾아서 GE 스펙 적용
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
		if (TargetASC)
		{
			// 이펙트 배달 완료! (ExecCalcCombat이 이어서 데미지 계산 및 텍스트 출력을 수행합니다)
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
		}
	}

	// (선택) 여기에 파티클(폭발)이나 사운드 스폰 로직 추가 가능

	// 적을 맞췄으니 스스로 풀로 돌아감
	ReturnSelfToPool();
}

void AProjectileBase::ReturnSelfToPool()
{
	if (UWorld* World = GetWorld())
	{
		if (UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>())
		{
			PoolSubsystem->ReturnToPool(this);
		}
		else
		{
			Destroy();
		}
	}
}