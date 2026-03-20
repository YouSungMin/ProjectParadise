// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/AIUnit/UnitBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Data/Assets/FXDataAsset.h"

AUnitBase::AUnitBase()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bIsDead = false;

	StimuliSourceComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSourceComp"));

	//0304 김성현 - 시야(Sight) 감지 대상으로 자동 등록 세팅
	if (StimuliSourceComp)
	{
		// 스폰 시 자동으로 퍼셉션 시스템에 등록되도록 설정
		StimuliSourceComp->RegisterWithPerceptionSystem();

		//이 액터를 시각 감지 대상으로 등록합니다.
		StimuliSourceComp->RegisterForSense(TSubclassOf<UAISense_Sight>());
	}

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));
}

void AUnitBase::OnPoolDeactivate_Implementation()
{
	// 타이머 및 AI 중지
	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (AIC->GetBrainComponent()) AIC->GetBrainComponent()->StopLogic("Returned to Pool");

		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->ClearValue(FName("TargetActor"));
			BB->ClearValue(FName("TargetLocation"));
		}
		AIC->UnPossess();
	}

	FVector ExileLocation = FVector(0.f, 0.f, -100000.f);
	SetActorLocation(ExileLocation);

	// 물리 및 충돌 완전 차단
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	bIsDead = true;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	SetActorEnableCollision(false);
}

void AUnitBase::OnPoolActivate_Implementation()
{
	// 가시성 복구
	bIsDead = false;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// 물리 상태 초기화 (메시/캡슐)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetAllBodiesSimulatePhysics(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
		MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			AnimInst->StopAllMontages(0.0f);
		}
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetSimulatePhysics(false);
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Capsule->SetCollisionResponseToAllChannels(ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

		Capsule->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		Capsule->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	// 캐릭터 무브먼트 리셋
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->Velocity = FVector::ZeroVector;
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->CurrentFloor.Clear();
	}

	SetActorEnableCollision(true);
}

FCombatActionData AUnitBase::GetCombatActionData(ECombatActionType ActionType) const
{
	FCombatActionData Result;

	if (ActionType == ECombatActionType::BasicAttack)
	{
		Result = BasicAttackData;
	}

	return Result;
}

TArray<FFXPayload*> AUnitBase::GetFXPayloads(EFXEventType EventType) const
{
	TArray<FFXPayload*> ResultPayloads;

	// 요청 타입(Enum)에 따라 딱 필요한 에셋만 동기 로드합니다!
	switch (EventType)
	{
	case EFXEventType::Hit:
	{
		if (UFXDataAsset* ReactionAsset = CachedAIUnitFX.FXData.LoadSynchronous())
		{
			if (FFXPayload* Payload = ReactionAsset->FindEffect(CachedAIUnitFX.HitTag))
				ResultPayloads.Add(Payload);
		}
		break;
	}

	case EFXEventType::Death:
	{
		if (UFXDataAsset* ReactionAsset = CachedAIUnitFX.FXData.LoadSynchronous())
		{
			if (FFXPayload* Payload = ReactionAsset->FindEffect(CachedAIUnitFX.DeathTag))
				ResultPayloads.Add(Payload);
		}
		break;
	}

	case EFXEventType::BasicAttack:
	{
		if (UFXDataAsset* ActionAsset = CachedAIUnitFX.FXData.LoadSynchronous())
		{
			if (FFXPayload* Payload = ActionAsset->FindEffect(CachedAIUnitFX.BasicAttackTag))
				ResultPayloads.Add(Payload);
		}
		break;
	}

	case EFXEventType::BasicAttackHit:
	{
		if (UFXDataAsset* ActionAsset = CachedAIUnitFX.FXData.LoadSynchronous())
		{
			if (FFXPayload* Payload = ActionAsset->FindEffect(CachedAIUnitFX.BasicAttackHitTag))
				ResultPayloads.Add(Payload);
		}
		break;
	}

	case EFXEventType::Skill:
	case EFXEventType::SkillHit:
	case EFXEventType::Ultimate:
		// 스킬은 ASkillCasterUnit에서 처리하고, 몬스터는 궁극기가 없으므로 패스
		break;
	}

	return ResultPayloads;
}

void AUnitBase::InitializeUnit(FAIUnitStats* InStats, FAIUnitAssets* InAssets)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (InStats)
	{
		if (UBaseAttributeSet* BaseSet = Cast<UBaseAttributeSet>(AttributeSet))
		{
			BaseSet->InitMaxHealth(InStats->BaseMaxHP);
			BaseSet->InitHealth(BaseSet->GetMaxHealth());
			BaseSet->InitAttackPower(InStats->BaseAttackPower);
			BaseSet->InitDefense(InStats->BaseDefense);
			if (!InStats->BasicAttackActionHandle.IsNull())
			{
				if (FActionStats* ActionRow = InStats->BasicAttackActionHandle.GetRow<FActionStats>(TEXT("BasicAttackLookup")))
				{
					// 액션 테이블에 정의된 평타 사거리로 AttributeSet을 초기화합니다.
					BaseSet->InitAttackRange(ActionRow->AttackRange);
					BasicAttackData.Stats = *ActionRow; // 캐싱 변수도 함께 업데이트
					if (!ActionRow->ProjectileDataHandle.IsNull())
					{
						if (FProjectileStats* ProjRow = ActionRow->ProjectileDataHandle.GetRow<FProjectileStats>(TEXT("BasicAttackProjectileLookup")))
						{
							BasicAttackData.ProjectileStats = *ProjRow;
						}
					}
				}
				else
				{
					// 만약 데이터를 찾지 못했을 경우의 안전 장치
					BaseSet->InitAttackRange(150.0f);
					BasicAttackData.Stats.AttackRange = 150.0f;
					BasicAttackData.Stats.DamageMultiplier = 1.0f;
					BasicAttackData.Stats.AttackRadius = 40.0f;
					BasicAttackData.Stats.ForwardOffset = 0.0f;
				}
			}
		}


		// [Movement]
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = InStats->BaseMoveSpeed;
		}
	}
	if (InAssets)
	{
		UnitAssets = *InAssets;

		// 유닛 크기 설정
		SetActorScale3D(FVector(InAssets->Scale));

		// 스켈레탈 메시 로드 및 적용
		if (!InAssets->SkeletalMesh.IsNull())
		{
			USkeletalMesh* LoadedMesh = InAssets->SkeletalMesh.LoadSynchronous();
			if (LoadedMesh) GetMesh()->SetSkeletalMesh(LoadedMesh);
		}

		// 애니메이션 블루프린트 설정
		if (InAssets->AnimBlueprint)
		{
			GetMesh()->SetAnimInstanceClass(InAssets->AnimBlueprint);
		}

		// 전투 데이터 캐싱 (멤버 변수에 저장)
		BasicAttackData.EffectClass = InAssets->BasicAttackSetup.EffectClass;
		BasicAttackData.MontageToPlay = InAssets->AttackMontage.LoadSynchronous(); // 미리 로드해둠
		BasicAttackData.ProjectileClass = InAssets->BasicAttackSetup.ProjectileClass;
		CachedDeathMontage = InAssets->DeathMontage.LoadSynchronous();
		CachedHitMontage = InAssets->HitMontage.LoadSynchronous();
		CachedAIUnitFX = InAssets->AIUnitFX;
		this->FactionTag = InAssets->FactionTag;



		// ⭐ 중요: GAS 시스템에도 태그 등록 (타겟팅/필터링용)
		if (AbilitySystemComponent && FactionTag.IsValid())
		{
			// 기존 태그가 있다면 제거 (재활용 시 꼬임 방지)
			AbilitySystemComponent->RemoveLooseGameplayTag(FactionTag);

			// 새 태그 등록
			AbilitySystemComponent->AddLooseGameplayTag(FactionTag);
		}


		// 어빌리티 부여 (Grant Ability)
		if (AbilitySystemComponent)
		{
			// 평타 (Basic Ability)
			if (BasicAbilityHandle.IsValid())
			{
				AbilitySystemComponent->ClearAbility(BasicAbilityHandle); // 재사용 시 초기화
			}

			if (InAssets->BasicAttackSetup.AbilityClass)
			{
				FGameplayAbilitySpec Spec(InAssets->BasicAttackSetup.AbilityClass, 1, -1);
				BasicAbilityHandle = AbilitySystemComponent->GiveAbility(Spec);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] Initialized. Faction: %s"), *GetName(), *FactionTag.ToString());
}

void AUnitBase::Die()
{
	Super::Die();

	// 안전장치 타이머 가동
	GetWorldTimerManager().SetTimer(
		FailSafeDestroyTimerHandle,
		this,
		&AUnitBase::ExecuteReturnToPool,
		5.0f,
		false
	);
}

bool AUnitBase::IsEnemy(AUnitBase* OtherUnit)
{
	if (!OtherUnit || OtherUnit == this) return false;
	// 태그가 다르면 적군으로 간주
	return !this->FactionTag.MatchesTag(OtherUnit->FactionTag);
}

void AUnitBase::PlayRangeAttack()
{
	// 공격 몽타주 실행 또는 발사체 생성 로직
	UE_LOG(LogTemp, Log, TEXT("%s 유닛이 원거리 공격을 수행합니다."), *GetName());
}

UAnimMontage* AUnitBase::GetDeathMontage() const
{
	return CachedDeathMontage;
}

UAnimMontage* AUnitBase::GetHitMontage() const
{
	return CachedHitMontage;
}

void AUnitBase::OnDeathAnimationFinished()
{
	ExecuteReturnToPool();
}

void AUnitBase::ExecuteReturnToPool()
{
	GetWorldTimerManager().ClearTimer(FailSafeDestroyTimerHandle);

	// 풀 반환 (오브젝트 풀링 서브시스템 활용)
	if (UWorld* World = GetWorld())
	{
		if (UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>())
		{
			PoolSubsystem->ReturnToPool(this);
		}
	}
}

void AUnitBase::SetAvoidanceEnabled(bool bEnable)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseRVOAvoidance = bEnable;
	}
}