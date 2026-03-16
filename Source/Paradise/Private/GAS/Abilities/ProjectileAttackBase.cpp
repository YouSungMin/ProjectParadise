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

	// 투사체 클래스가 비어있으면 에러
	if (!CombatData.ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [ProjectileAttackBase] 발사할 ProjectileClass가 없습니다!"));
		return;
	}

	FTransform MuzzleTransform = AvatarChar->GetCurrentMuzzleTransform();

	// 소켓의 월드 좌표 가져오기
	FVector SpawnLocation = MuzzleTransform.GetLocation();

	// 기존처럼 캐릭터가 바라보는 앞으로 Offset만큼 띄워주기
	SpawnLocation += AvatarChar->GetActorForwardVector() * CombatData.Stats.ForwardOffset;

	// 캐릭터가 보는 방향으로 발사
	FRotator BaseRotation = AvatarChar->GetActorRotation();

	int32 PCount = FMath::Max(1, CombatData.ProjectileStats.ProjectileCount);
	float SAngle = CombatData.ProjectileStats.SpreadAngle;

	// 각도 간격 계산: 1발이면 0도, 여러 발이면 전체 각도를 (발사수-1)로 나눔
	float AngleStep = (PCount > 1) ? (SAngle / (PCount - 1)) : 0.0f;

	// 첫 번째 투사체가 쏘아질 시작 각도 (부채꼴의 가장 왼쪽 끝)
	float StartAngle = (PCount > 1) ? -(SAngle / 2.0f) : 0.0f;

	// 스폰 파라미터 설정
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarChar;
	SpawnParams.Instigator = AvatarChar;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
	{
		// 💡 데이터 테이블에 적힌 발사 개수(PCount)만큼 반복해서 스폰합니다!
		for (int32 i = 0; i < PCount; ++i)
		{
			// 기준 회전(앞)에서 계산된 각도만큼 Yaw(좌우)를 비틂
			FRotator PelletRotation = BaseRotation;
			PelletRotation.Yaw += StartAngle + (AngleStep * i);

			AActor* SpawnedProjectile = PoolSubsystem->SpawnPoolActor<AActor>(
				CombatData.ProjectileClass,
				SpawnLocation,
				PelletRotation, // 💡 계산된 부채꼴 각도 적용!
				AvatarChar,
				AvatarChar
			);

			if (SpawnedProjectile)
			{
				if (AProjectileBase* Proj = Cast<AProjectileBase>(SpawnedProjectile))
				{
					// [중요] 오브젝트 풀링 버그 방지를 위한 오너 확실한 재각인!
					Proj->SetOwner(AvatarChar);
					Proj->SetInstigator(AvatarChar);

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

					// 💡 변경: 투사체의 속도를 ProjectileStats에서 꺼내옵니다!
					Proj->ApplyCombatData(
						CombatData.Stats.AttackRange,
						CombatData.Stats.AttackRadius,
						CombatData.ProjectileStats
					);
				}
			}
		}
	}
}