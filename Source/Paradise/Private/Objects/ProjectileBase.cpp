// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
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

	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	NiagaraComp->SetupAttachment(SphereComp);

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
	ProjectileMovementComp->InitialSpeed = 1000.0f;
	ProjectileMovementComp->MaxSpeed = 2000.0f;
	ProjectileMovementComp->ProjectileGravityScale = 0.0f;
	// 풀링을 위해 자동 활성화를 끕니다. (OnPoolActivate에서 켭니다)
	ProjectileMovementComp->bAutoActivate = false;
}

void AProjectileBase::OnPoolActivate_Implementation()
{
	// 1. 액터 보이기 & 충돌 켜기
	SetActorHiddenInGame(false);
	if (SphereComp) SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 2. 나이아가라 파티클 처음부터 재생 (스태틱 매쉬 대신)
	if (NiagaraComp) NiagaraComp->Activate(true);

	// 3. 이동 컴포넌트 재시작 및 발사 방향 리셋
	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->SetComponentTickEnabled(true);
		ProjectileMovementComp->Activate(true);
		// 현재 바라보는 방향으로 속도 재설정
		ProjectileMovementComp->Velocity = GetActorForwardVector() * ProjectileMovementComp->InitialSpeed;
	}

	// 4. 수명 타이머 세팅
	if (LifeTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(LifeTimerHandle, this, &AProjectileBase::ReturnSelfToPool, LifeTime, false);
	}
}

void AProjectileBase::OnPoolDeactivate_Implementation()
{
	// 타이머 초기화
	GetWorld()->GetTimerManager().ClearTimer(LifeTimerHandle);

	// 물리/이동 정지 및 숨김
	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->Deactivate();
		ProjectileMovementComp->SetComponentTickEnabled(false);
		ProjectileMovementComp->Velocity = FVector::ZeroVector;
	}

	if (SphereComp) SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);

	if (NiagaraComp) NiagaraComp->Deactivate();

	// 갖고 있던 데미지 정보 파기
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

bool AProjectileBase::IsValidTarget(AActor* OtherActor)
{
	// 예외 대상 무시
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator())
		return false;

	if (ACharacterBase* Shooter = Cast<ACharacterBase>(GetInstigator()))
	{
		if (ACharacterBase* TargetChar = Cast<ACharacterBase>(OtherActor))
		{
			if (!Shooter->IsHostile(TargetChar))
			{
				return false;
			}
		}
	}

	return true;
}
