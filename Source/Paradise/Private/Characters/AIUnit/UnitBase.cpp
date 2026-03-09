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
		Result.MontageToPlay = BasicAttackData.MontageToPlay;
		Result.DamageEffectClass = BasicAttackData.DamageEffectClass;
		Result.ProjectileClass = BasicAttackData.ProjectileClass;
		Result.DamageMultiplier = BasicAttackData.DamageMultiplier;
		Result.AttackRange = BasicAttackData.AttackRange;
		Result.AttackRadius = BasicAttackData.AttackRadius;
		Result.ForwardOffset = BasicAttackData.ForwardOffset;
	}
	else if (ActionType == ECombatActionType::WeaponSkill)
	{
		// 몬스터 전용 스킬 로직이 필요하다면 여기서 분기 처리
		// 예: Result.MontageToPlay = CachedSkillMontage;
	}


	return Result;
}

FFXPayload* AUnitBase::GetFXPayload(EFXEventType EventType) const
{
	UFXDataAsset* TargetAsset = nullptr;
	FGameplayTag TargetTag;

	// 몬스터는 궁극기(Ultimate)가 없으므로 Reaction과 Action만 처리합니다.
	switch (EventType)
	{
	case EFXEventType::Hit:
		TargetAsset = CachedReactionFX.ReactionFXData.LoadSynchronous();
		TargetTag = CachedReactionFX.HitTag;
		break;
	case EFXEventType::Death:
		TargetAsset = CachedReactionFX.ReactionFXData.LoadSynchronous();
		TargetTag = CachedReactionFX.DeathTag;
		break;
	case EFXEventType::BasicAttack:
		TargetAsset = CachedActionFX.ActionFXData.LoadSynchronous();
		TargetTag = CachedActionFX.BasicAttackTag;
		break;
	case EFXEventType::Skill:
		//TargetAsset = CachedActionFX.ActionFXData.LoadSynchronous();
		//TargetTag = CachedActionFX.SkillTag;
		break;
	case EFXEventType::Ultimate:
		return nullptr; // 몬스터는 궁극기가 없음
	}

	if (TargetAsset && TargetTag.IsValid())
	{
		return TargetAsset->FindEffect(TargetTag);
	}

	return nullptr;
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
			if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance()))
			{
				if (FActionStats* ActionRow = GI->GetDataTableRow<FActionStats>(GI->ActionStatsDataTable, InStats->BasicAttackActionID))
				{
					// 액션 테이블에 정의된 평타 사거리로 AttributeSet을 초기화합니다.
					BaseSet->InitAttackRange(ActionRow->AttackRange);
					BasicAttackData.AttackRange = ActionRow->AttackRange; // 캐싱 변수도 함께 업데이트
					BasicAttackData.DamageMultiplier = ActionRow->DamageMultiplier;
					BasicAttackData.AttackRadius = ActionRow->AttackRadius;
					BasicAttackData.ForwardOffset = ActionRow->ForwardOffset;
				}
				else
				{
					// 만약 데이터를 찾지 못했을 경우의 안전 장치
					BaseSet->InitAttackRange(150.0f);
					BasicAttackData.AttackRange = 150.0f;
					BasicAttackData.DamageMultiplier = 1.0f;

					BasicAttackData.AttackRadius = 40.0f;
					BasicAttackData.ForwardOffset = 0.0f;
				}
			}
		}

		// ⭐ 중요: GAS 시스템에도 태그 등록 (타겟팅/필터링용)
		if (AbilitySystemComponent && FactionTag.IsValid())
		{
			// 기존 태그가 있다면 제거 (재활용 시 꼬임 방지)
			AbilitySystemComponent->RemoveLooseGameplayTag(FactionTag);

			// 새 태그 등록
			AbilitySystemComponent->AddLooseGameplayTag(FactionTag);
		}

		// [Movement]
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = InStats->BaseMoveSpeed;
		}
	}
	if (InAssets)
	{
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
		BasicAttackData.DamageEffectClass = InAssets->BasicAttackSetup.EffectClass;
		BasicAttackData.MontageToPlay = InAssets->AttackMontage.LoadSynchronous(); // 미리 로드해둠
		BasicAttackData.ProjectileClass = InAssets->BasicAttackSetup.ProjectileClass;
		CachedDeathMontage = InAssets->DeathMontage.LoadSynchronous();
		CachedHitMontage = InAssets->HitMontage.LoadSynchronous();
		CachedReactionFX = InAssets->ReactionFX; // 피격/사망 블록 캐싱
		CachedActionFX = InAssets->ActionFX;  // 공격 연출 블록 캐싱
		this->FactionTag = InAssets->FactionTag;

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