// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Characters/Base/CharacterBase.h"
#include "Kismet/KismetSystemLibrary.h"

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

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComp"));
	StaticMeshComp->SetupAttachment(SphereComp);
	StaticMeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	StaticMeshComp->SetGenerateOverlapEvents(false);

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
	// 풀에서 꺼내 질때 타격 명탄 및 관통 횟수 초기화
	HitActors.Empty();
	CurrentPierceCount = 0;

	// 액터 보이기 & 충돌 켜기
	SetActorHiddenInGame(false);
	if (SphereComp) SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (StaticMeshComp) StaticMeshComp->SetVisibility(true);

	// 나이아가라 파티클 재생 
	if (NiagaraComp) NiagaraComp->Activate(true);

	// 이동 컴포넌트 재시작 및 발사 방향 리셋
	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->SetComponentTickEnabled(true);
		ProjectileMovementComp->Activate(true);
		// 현재 바라보는 방향으로 속도 재설정
		ProjectileMovementComp->Velocity = GetActorForwardVector() * ProjectileMovementComp->InitialSpeed;
	}

	// 수명 타이머 세팅
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

	if (StaticMeshComp) StaticMeshComp->SetVisibility(false);

	if (NiagaraComp) NiagaraComp->Deactivate();

	// 갖고 있던 데미지 정보 파기
	DamageSpecHandle = FGameplayEffectSpecHandle();
}

void AProjectileBase::SetDamageSpecHandle(const FGameplayEffectSpecHandle& InSpecHandle)
{
	DamageSpecHandle = InSpecHandle;
}

void AProjectileBase::ApplyCombatData(float InAttackRange, float InAttackRadius, const FProjectileStats& InProjStats)
{
	CachedProjStats = InProjStats; // 스탯 캐싱

	// 속도 적용
	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->InitialSpeed = CachedProjStats.ProjectileSpeed;
		ProjectileMovementComp->MaxSpeed = CachedProjStats.ProjectileSpeed;
	}

	// 사거리(거리)를 기반으로 생존 시간(LifeTime) 계산 (시간 = 거리 / 속력)
	float CurrentSpeed = ProjectileMovementComp->InitialSpeed;
	if (CurrentSpeed > 0.0f)
	{
		LifeTime = InAttackRange / CurrentSpeed;
	}
	//UE_LOG(LogTemp, Warning, TEXT("⏱️ [Projectile] 사거리: %.1f -> 계산된 수명: %.3f초"), InAttackRange, LifeTime);
	
	// 타이머 재설정
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

void AProjectileBase::ApplyDamageToTarget(AActor* TargetActor)
{
	if (DamageSpecHandle.IsValid())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
		}
	}
}

void AProjectileBase::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 자기 자신이나, 나를 쏜 주인(Instigator)은 무시
	if (!IsValidTarget(OtherActor)) return;

	// 이미 맞은 적 무시 (다단히트 방지)
	if (HitActors.Contains(OtherActor)) return;

	// 타격 명단 등록 및 데미지 적용
	HitActors.Add(OtherActor);

	// ==========================================================
	// 폭발 기믹 검사 (스플래시 데미지)
	// ==========================================================
	if (CachedProjStats.ExplosionRadius > 0.0f)
	{
		// 폭발 범위에 있는 대상들을 모을 배열
		TArray<AActor*> OverlappedActors;
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

		// 나와 내 주인만 무시 (주 타겟 OtherActor는 무시하지 않음!)
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);
		IgnoredActors.Add(GetInstigator());

		// 🌟 제자리 폭발은 OverlapActors 사용!
		UKismetSystemLibrary::SphereOverlapActors(
			this,
			GetActorLocation(),
			CachedProjStats.ExplosionRadius,
			ObjectTypes,
			nullptr,
			IgnoredActors,
			OverlappedActors
		);

		// 범위 안의 모든 적에게 데미지 적용
		for (AActor* HitActor : OverlappedActors)
		{
			if (IsValidTarget(HitActor))
			{
				ApplyDamageToTarget(HitActor);
			}
		}

		// (디버그) 폭발 범위 시각화 - 확인 후 주석 처리하세요
		// DrawDebugSphere(GetWorld(), GetActorLocation(), CachedProjStats.ExplosionRadius, 32, FColor::Orange, false, 1.0f);

		// 폭발했으면 무조건 소멸
		ReturnSelfToPool();
		return;
	}
	else
	{
		// 폭발 기믹이 없을 때만 방금 닿은 OtherActor에게 단일 데미지 적용
		ApplyDamageToTarget(OtherActor);
	}

	// ==========================================================
	// 관통 기믹 검사
	// ==========================================================
	if (CurrentPierceCount < CachedProjStats.MaxPierceCount)
	{
		// 뚫고 지나가므로 카운트만 올리고 소멸시키지 않음
		CurrentPierceCount++;
	}
	else
	{
		// 관통 횟수를 다 썼거나 단일 타겟(MaxPierce=0)이면 소멸
		ReturnSelfToPool();
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

	if (OtherActor->IsA<AProjectileBase>())
	{
		return false;
	}

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
