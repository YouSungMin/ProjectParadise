// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/AIUnit/UnitBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/ObjectPoolSubsystem.h"

AUnitBase::AUnitBase()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bIsDead = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));
}

void AUnitBase::OnPoolActivate_Implementation()
{
	bIsDead = false;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	SetActorEnableCollision(true);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->Velocity = FVector::ZeroVector;
		MoveComp->SetMovementMode(MOVE_Walking);
	}
}

void AUnitBase::OnPoolDeactivate_Implementation()
{
	// 풀로 돌아갈 때 AI 로직 중지 및 컨트롤러 해제
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (AIC->GetBrainComponent())
		{
			AIC->GetBrainComponent()->StopLogic("Returned to Pool");
		}
		AIC->UnPossess();
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

FCombatActionData AUnitBase::GetCombatActionData(ECombatActionType ActionType) const
{
	FCombatActionData Result;

	if (ActionType == ECombatActionType::BasicAttack)
	{
		Result.MontageToPlay = CachedAttackMontage;
		Result.DamageEffectClass = CachedDamageEffectClass;
		Result.ProjectileClass = CachedProjectileClass;
		Result.DamageMultiplier = CachedDamageMultiplier;
		Result.AttackRange = CachedAttackRange;
		Result.AttackRadius = CachedAttackRadius;
		Result.ForwardOffset = CachedForwardOffset;
	}
	else if (ActionType == ECombatActionType::WeaponSkill)
	{
		// 몬스터 전용 스킬 로직이 필요하다면 여기서 분기 처리
		// 예: Result.MontageToPlay = CachedSkillMontage;
	}


	return Result;
}

void AUnitBase::InitializeUnit(FAIUnitStats* InStats, FAIUnitAssets* InAssets)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (InStats)
	{
		CachedBasicAttackActionID = InStats->BasicAttackActionID;
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
					CachedAttackRange = ActionRow->AttackRange; // 캐싱 변수도 함께 업데이트
					CachedDamageMultiplier = ActionRow->DamageMultiplier;
					CachedAttackRadius = ActionRow->AttackRadius;
					CachedForwardOffset = ActionRow->ForwardOffset;
				}
				else
				{
					// 만약 데이터를 찾지 못했을 경우의 안전 장치
					BaseSet->InitAttackRange(150.0f);
					CachedAttackRange = 150.0f;
					CachedDamageMultiplier = 1.0f;

					CachedAttackRadius = 40.0f;
					CachedForwardOffset = 0.0f;
				}
			}
		}

		// ✅ [Tag] 소속 태그 적용 (데이터 테이블 -> 멤버 변수)
		this->FactionTag = InStats->FactionTag;

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
		CachedDamageEffectClass = InAssets->BasicAttackEffect;
		CachedAttackMontage = InAssets->AttackMontage.LoadSynchronous(); // 미리 로드해둠
		CachedProjectileClass = InAssets->ProjectileClass;

		// 어빌리티 부여 (Grant Ability)
		if (AbilitySystemComponent)
		{
			// 평타 (Basic Ability)
			if (BasicAbilityHandle.IsValid())
			{
				AbilitySystemComponent->ClearAbility(BasicAbilityHandle); // 재사용 시 초기화
			}

			if (InAssets->BasicAbility)
			{
				FGameplayAbilitySpec Spec(InAssets->BasicAbility, 1, -1);
				BasicAbilityHandle = AbilitySystemComponent->GiveAbility(Spec);
			}

			// 스킬 (Skills) - 보스 사용
			for (const auto& Handle : SkillAbilityHandles)
			{
				AbilitySystemComponent->ClearAbility(Handle);
			}
			SkillAbilityHandles.Empty();

			for (const auto& SkillClass : InAssets->SkillAbilities)
			{
				if (SkillClass)
				{
					FGameplayAbilitySpec Spec(SkillClass, 1, -1);
					SkillAbilityHandles.Add(AbilitySystemComponent->GiveAbility(Spec));
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] Initialized. Faction: %s"), *GetName(), *FactionTag.ToString());
}

void AUnitBase::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	if (UWorld* World = GetWorld())
	{
		if (UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>())
		{
			// 사망 시 풀로 반환
			PoolSubsystem->ReturnToPool(this);
		}
	}
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

void AUnitBase::SetAvoidanceEnabled(bool bEnable)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseRVOAvoidance = bEnable;
	}
}