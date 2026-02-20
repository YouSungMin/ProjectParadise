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
	ProjectileMovementComp->InitialSpeed = 2000.0f;
	ProjectileMovementComp->MaxSpeed = 2000.0f;
	ProjectileMovementComp->ProjectileGravityScale = 0.0f;
	// 풀링을 위해 자동 활성화를 끕니다. (OnPoolActivate에서 켭니다)
	ProjectileMovementComp->bAutoActivate = false;
}

void AProjectileBase::OnPoolActivate_Implementation()
{
	SetActorHiddenInGame(false);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

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

void AProjectileBase::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 자기 자신이나, 나를 쏜 주인(Instigator)은 무시
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator()) return;

	// 주인이 ACharacterBase이고, 맞은 대상도 ACharacterBase일 때 적군인지 확인
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