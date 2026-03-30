// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ProjectileAttackBase.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Objects/ProjectileBase.h"
#include "Characters/Base/CharacterBase.h"
#include "TimerManager.h"

UProjectileAttackBase::UProjectileAttackBase()
{
	// 기본적으로 감지할 태그 설정
	FireEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Fire"));
}

void UProjectileAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	bIsShooting = false;
	bMontageFinished = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 코스트 및 쿨타임 확인 (Commit)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FCombatActionData CombatData = GetCombatDataFromActor();

	if (ACharacterBase* AvatarChar = Cast<ACharacterBase>(GetAvatarActorFromActorInfo()))
	{
		AvatarChar->SetCurrentActionData(CombatData);
	}

	if (!CombatData.MontageToPlay)
	{
		//UE_LOG(LogTemp, Warning, TEXT("❌ [ProjectileAttackBase] 재생할 몽타주가 없습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}if (ACharacterBase* AvatarChar = Cast<ACharacterBase>(GetAvatarActorFromActorInfo()))
	{
		AvatarChar->SetCurrentActionData(CombatData);
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

void UProjectileAttackBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	bIsShooting = false;
	bMontageFinished = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UProjectileAttackBase::OnMontageCompleted()
{
	bMontageFinished = true; // 애니메이션 종료 기록

	if (!bIsShooting)
	{
		Super::OnMontageCompleted();
	}
}

void UProjectileAttackBase::OnGameplayEventReceived(FGameplayEventData Payload)
{
	ACharacterBase* AvatarChar = Cast<ACharacterBase>(GetAvatarActorFromActorInfo());
	if (!AvatarChar) return;

	FCombatActionData CombatData = GetCombatDataFromActor();

	// 투사체 클래스가 비어있으면 에러
	if (!CombatData.ProjectileClass)
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [ProjectileAttackBase] 발사할 ProjectileClass가 없습니다!"));
		return;
	}

	BurstCurrentCount = 0; // 발사 카운트 초기화

	if (CombatData.ProjectileStats.BurstDelay <= 0.0f)
	{
		// 샷건 모드 (한 번에 다 스폰)
		int32 PCount = FMath::Max(1, CombatData.ProjectileStats.ProjectileCount);
		for (int32 i = 0; i < PCount; ++i)
		{
			FireSinglePellet();
		}

		if (bMontageFinished)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
	}
	else
	{
		// [머신건 모드]
		bIsShooting = true;

		FTimerHandle BurstTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			BurstTimerHandle,
			this,
			&UProjectileAttackBase::FireSinglePellet,
			CombatData.ProjectileStats.BurstDelay,
			true,  // 반복 켜기
			0.0f   // 첫 발 즉시 발사
		);
	}
}

void UProjectileAttackBase::FireSinglePellet()
{
	ACharacterBase* AvatarChar = Cast<ACharacterBase>(GetAvatarActorFromActorInfo());
	if (!AvatarChar) return;

	FCombatActionData CombatData = GetCombatDataFromActor();

	int32 TotalCount = FMath::Max(1, CombatData.ProjectileStats.ProjectileCount);
	float SAngle = CombatData.ProjectileStats.SpreadAngle;

	float AngleStep = (TotalCount > 1) ? (SAngle / (TotalCount - 1)) : 0.0f;
	float StartAngle = (TotalCount > 1) ? -(SAngle / 2.0f) : 0.0f;

	// 스폰 위치 계산
	FTransform MuzzleTransform = AvatarChar->GetCurrentMuzzleTransform();
	FVector SpawnLocation = MuzzleTransform.GetLocation();
	SpawnLocation += AvatarChar->GetActorForwardVector() * CombatData.Stats.ForwardOffset;

	FRotator BaseRotation = AvatarChar->GetActorRotation();

	// 부채꼴 각도 적용 (현재 발사된 카운트인 BurstCurrentCount에 맞춰서 벌어짐)
	FRotator PelletRotation = BaseRotation;
	PelletRotation.Yaw += StartAngle + (AngleStep * BurstCurrentCount);

	// 랜덤 탄퍼짐 오차 적용
	if (CombatData.ProjectileStats.RandomSpreadAngle > 0.0f)
	{
		float RandomPitch = FMath::RandRange(-CombatData.ProjectileStats.RandomSpreadAngle, CombatData.ProjectileStats.RandomSpreadAngle);
		float RandomYaw = FMath::RandRange(-CombatData.ProjectileStats.RandomSpreadAngle, CombatData.ProjectileStats.RandomSpreadAngle);

		PelletRotation.Pitch += RandomPitch;
		PelletRotation.Yaw += RandomYaw;
	}

	// 스폰 및 데이터 적용
	if (UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
	{
		AActor* SpawnedProjectile = PoolSubsystem->SpawnPoolActor<AActor>(
			CombatData.ProjectileClass,
			SpawnLocation,
			PelletRotation,
			AvatarChar,
			AvatarChar
		);

		if (AProjectileBase* Proj = Cast<AProjectileBase>(SpawnedProjectile))
		{
			Proj->SetOwner(AvatarChar);
			Proj->SetInstigator(AvatarChar);

			if (CombatData.EffectClass)
			{
				FGameplayEffectSpecHandle SpecHandle = MakeSpecHandle(CombatData.EffectClass, GetAbilityLevel());
				SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.Multiplier")), CombatData.Stats.DamageMultiplier);

				FGameplayEffectContextHandle ContextHandle = SpecHandle.Data->GetContext();
				ContextHandle.AddInstigator(AvatarChar, Proj);

				Proj->SetDamageSpecHandle(SpecHandle);
			}

			// 투사체 속성 넘기기
			Proj->ApplyCombatData(CombatData.Stats.AttackRange, CombatData.Stats.AttackRadius, CombatData.ProjectileStats);
		}
	}

	// 한 발 쐈으니 카운트 증가
	BurstCurrentCount++;

	// 총 발사 개수(TotalCount)만큼 다 쐈으면 타이머 종료
	if (BurstCurrentCount >= TotalCount)
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
		bIsShooting = false;

		// 연사가 끝났는데 애니메이션도 다 끝났다면 어빌리티 종료
		if (bMontageFinished)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
	}
}
