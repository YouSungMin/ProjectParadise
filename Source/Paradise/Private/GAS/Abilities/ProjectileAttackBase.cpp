// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ProjectileAttackBase.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Objects/ProjectileBase.h"
#include "Characters/Base/CharacterBase.h"

UProjectileAttackBase::UProjectileAttackBase()
{
	// 기본적으로 감지할 태그 설정
	FireEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Fire"));
}

void UProjectileAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 코스트 및 쿨타임 확인 (Commit)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FCombatActionData CombatData = GetCombatDataFromActor();

	if (!CombatData.MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ [ProjectileAttackBase] 재생할 몽타주가 없습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 부모의 공용 함수로 몽타주 재생
	PlayMontageAndWaitCallback(CombatData.MontageToPlay);

	// 발사 이벤트(노티파이) 대기
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FireEventTag, nullptr, false, false
	);

	EventTask->EventReceived.AddDynamic(this, &UProjectileAttackBase::OnGameplayEventReceived);
	EventTask->ReadyForActivation();
}

void UProjectileAttackBase::OnGameplayEventReceived(FGameplayEventData Payload)
{
	ACharacterBase* AvatarChar = Cast<ACharacterBase>(GetAvatarActorFromActorInfo());
	if (!AvatarChar) return;

	FCombatActionData CombatData = GetCombatDataFromActor();

	UE_LOG(LogTemp, Warning, TEXT("🏹 [ProjectileAttackBase] 투사체 생성 직전! 읽어온 투사체 속도: %.1f"), CombatData.Stats.ProjectileSpeed);

	// 투사체 클래스가 비어있으면 에러
	if (!CombatData.ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [ProjectileAttackBase] 발사할 ProjectileClass가 없습니다!"));
		return;
	}

	// 발사 위치(Transform) 계산
	FVector SpawnLocation = AvatarChar->GetMuzzleLocation(MuzzleSocketName);
	SpawnLocation += AvatarChar->GetActorForwardVector() * CombatData.Stats.ForwardOffset;
	FRotator SpawnRotation = AvatarChar->GetActorRotation(); // 임시로 캐릭터가 보는 방향

	// 스폰 파라미터 설정
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarChar;
	SpawnParams.Instigator = AvatarChar;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UE_LOG(LogTemp, Warning, TEXT("🎯 [Debug] 투사체 발사 위치(MuzzleSocket): %s"), *SpawnLocation.ToString());
	// 투사체 스폰
	AActor* SpawnedProjectile = nullptr;

	if (UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
	{
		SpawnedProjectile = PoolSubsystem->SpawnPoolActor<AActor>(
			CombatData.ProjectileClass,
			SpawnLocation,
			SpawnRotation,
			AvatarChar,  // Owner
			AvatarChar   // Instigator
		);
	}

	if (SpawnedProjectile)
	{
		if (AProjectileBase* Proj = Cast<AProjectileBase>(SpawnedProjectile))
		{
			// 데미지 이펙트 세팅
			if (CombatData.EffectClass)
			{
				FGameplayEffectSpecHandle SpecHandle = MakeSpecHandle(CombatData.EffectClass, GetAbilityLevel());
				SpecHandle.Data->SetSetByCallerMagnitude(
					FGameplayTag::RequestGameplayTag(FName("Data.Damage.Multiplier")),
					CombatData.Stats.DamageMultiplier
				);
				Proj->SetDamageSpecHandle(SpecHandle);
			}

			// 사거리(길이)와 반경(두께) 전달하여 투사체 갱신
			Proj->ApplyCombatData(CombatData.Stats.AttackRange, CombatData.Stats.AttackRadius, CombatData.Stats.ProjectileSpeed);
		}
	}
}