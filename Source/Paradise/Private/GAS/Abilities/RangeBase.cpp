// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/RangeBase.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Objects/ProjectileBase.h"

URangeBase::URangeBase()
{
	// 기본적으로 감지할 태그 설정
	FireEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Fire"));
}

void URangeBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
		UE_LOG(LogTemp, Warning, TEXT("❌ [RangeBase] 재생할 몽타주가 없습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 부모의 공용 함수로 몽타주 재생
	PlayMontageAndWaitCallback(CombatData.MontageToPlay);

	// 발사 이벤트(노티파이) 대기
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FireEventTag, nullptr, false, false
	);

	EventTask->EventReceived.AddDynamic(this, &URangeBase::OnGameplayEventReceived);
	EventTask->ReadyForActivation();
}

void URangeBase::OnGameplayEventReceived(FGameplayEventData Payload)
{
	ACharacter* AvatarChar = GetPlayerCharacterFromActorInfo();
	if (!AvatarChar) return;

	FCombatActionData CombatData = GetCombatDataFromActor();

	// 투사체 클래스가 비어있으면 에러
	if (!CombatData.ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [RangeBase] 발사할 ProjectileClass가 없습니다!"));
		return;
	}

	// 발사 위치(Transform) 계산
	FVector SpawnLocation = AvatarChar->GetMesh()->GetSocketLocation(MuzzleSocketName);
	FRotator SpawnRotation = AvatarChar->GetActorRotation(); // 임시로 캐릭터가 보는 방향

	// (선택) 타겟팅 시스템이 있다면 타겟 방향으로 Rotation을 돌려주면 유도탄처럼 날아갑니다.

	// 스폰 파라미터 설정
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarChar;
	SpawnParams.Instigator = AvatarChar;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

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

	if (SpawnedProjectile && CombatData.DamageEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeSpecHandle(CombatData.DamageEffectClass, GetAbilityLevel());
		SpecHandle.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(FName("Data.Damage.Multiplier")),
			CombatData.DamageMultiplier
		);

		if (AProjectileBase* Proj = Cast<AProjectileBase>(SpawnedProjectile))
		{
			Proj->SetDamageSpecHandle(SpecHandle);
		}
	}
}