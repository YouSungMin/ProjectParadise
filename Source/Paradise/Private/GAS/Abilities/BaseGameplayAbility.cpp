// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/BaseGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "Characters/AIUnit/SkillCasterUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/InGame/InGameController.h"
#include "UI/HUD/Ingame/InGameHUDWidget.h"
#include "UI/Widgets/InGame/VirtualJoystickWidget.h"
#include "UI/Panel/Ingame/ActionControlPanel.h"

UBaseGameplayAbility::UBaseGameplayAbility()
{
	// 액터마다 별도의 어빌리티 인스턴스를 생성하도록 설정
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

ACharacter* UBaseGameplayAbility::GetPlayerCharacterFromActorInfo() const
{
	if (!CurrentActorInfo || !CurrentActorInfo->AvatarActor.IsValid())
	{
		return nullptr;
	}
	return Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get());
}

AController* UBaseGameplayAbility::GetPlayerControllerFromActorInfo() const
{
	if (!CurrentActorInfo) return nullptr;
	return CurrentActorInfo->PlayerController.Get();
}

FGameplayEffectSpecHandle UBaseGameplayAbility::MakeSpecHandle(TSubclassOf<UGameplayEffect> EffectClass, float Level)
{
	if (!EffectClass) return FGameplayEffectSpecHandle();

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return FGameplayEffectSpecHandle();

	// Context 생성: 이펙트의 출처(Instigator/Causer)를 기록
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this); // 이 어빌리티가 원인임을 명시

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	Context.AddInstigator(AvatarActor, AvatarActor);

	// Spec 생성
	return SourceASC->MakeOutgoingSpec(EffectClass, Level, Context);
}

void UBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	//0326 김성현 플레이어 차원에서 이동 방지
	if (ACharacter* Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		// 스킬 시작 시 걷고 있던 물리적 관성을 즉시 정지
		Char->GetCharacterMovement()->StopMovementImmediately();
	}

	// [추가] 03/23 담당자: 최지원, 어빌리티 시작 시 이동 차단
	//SetJoystickLocked(ActorInfo, true);
	// 어빌리티 시작 시 다른 액션 버튼 잠금
	SetActionButtonsLocked(ActorInfo, true);

	if (AbilityActionType == ECombatActionType::AIUnitSkill)
	{
		// 이 어빌리티를 실행한 주체가 ASkillCasterUnit(보스/캐스터)인지 확인
		if (ASkillCasterUnit* Caster = Cast<ASkillCasterUnit>(ActorInfo->AvatarActor.Get()))
		{
			if (FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
			{
				Caster->SetCurrentCastingSkillIndex(Spec->InputID);
			}
		}
	}
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// [추가] 03/23 담당자: 최지원, 어빌리티 종료 시 이동 재개
	//SetJoystickLocked(ActorInfo, false);
	// 어빌리티 종료 시 다른 액션 버튼 잠금 해제
	SetActionButtonsLocked(ActorInfo, false);

	// 스킬 시전이 끝났거나 캔슬되었을 때
	if (AbilityActionType == ECombatActionType::AIUnitSkill)
	{
		if (ASkillCasterUnit* Caster = Cast<ASkillCasterUnit>(ActorInfo->AvatarActor.Get()))
		{
			Caster->SetCurrentCastingSkillIndex(INDEX_NONE);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBaseGameplayAbility::ApplySpecHandleToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& SpecHandle)
{
	if (!TargetActor || !SpecHandle.IsValid()) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return;

	// Target의 ASC를 안전하게 찾기 (Interface 혹은 Component 검색)
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

	if (TargetASC)
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void UBaseGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (CooldownGE)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());

		FCombatActionData CombatData = GetCombatDataFromActorInfo(ActorInfo, Handle);

		if (CombatData.Stats.Cooldown > 0.0f)
		{
			FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(FName("Data.Cooldown"));
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(CooldownTag, CombatData.Stats.Cooldown);
		}
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

bool UBaseGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	// 기본 로직 검사 통과 못하면 실패
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	FCombatActionData CombatData = GetCombatDataFromActorInfo(ActorInfo, Handle);

	if (CombatData.Stats.ManaCost <= 0.0f) return true;

	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		float CurrentMana = ASC->GetNumericAttribute(UBaseAttributeSet::GetManaAttribute());
		if (CurrentMana < CombatData.Stats.ManaCost)
		{
			return false; // 마나 부족 실패
		}
	}
	return true;
}

void UBaseGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CostGE = GetCostGameplayEffect();
	if (CostGE)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CostGE->GetClass(), GetAbilityLevel());

		FCombatActionData CombatData = GetCombatDataFromActorInfo(ActorInfo, Handle);

		if (CombatData.Stats.ManaCost > 0.0f)
		{
			FGameplayTag CostTag = FGameplayTag::RequestGameplayTag(FName("Data.Cost.Mana"));
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(CostTag, -CombatData.Stats.ManaCost);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
}

UAbilityTask_PlayMontageAndWait* UBaseGameplayAbility::PlayMontageAndWaitCallback(UAnimMontage* MontageToPlay, FName TaskInstanceName)
{
	if (!MontageToPlay) return nullptr;

	ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!AvatarChar) return nullptr;

	USkeletalMeshComponent* Mesh = AvatarChar->GetMesh();
	if (!Mesh || !Mesh->GetAnimInstance())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [BaseGA] %s 의 AnimBP(AnimInstance)가 없습니다! 몽타주 재생 취소."), *AvatarChar->GetName());
		return nullptr;
	}

	float FinalPlayRate = 1.0f;

	// 캐싱된 전투 데이터에서 애니메이션 재생속도 가져오기
	float AnimSpeed = 1.0f;
	if (bIsDataCached)
	{
		AnimSpeed = CachedCombatData.Stats.AnimPlayRate;
		if (AnimSpeed <= 0.0f) AnimSpeed = 1.0f; // 방어 코드
	}

	// 어트리뷰트 셋의 현재 공격속도 가져오기
	float AttackSpeed = 1.0f;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		float FoundSpeed = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackSpeedAttribute());
		if (FoundSpeed > 0.0f)
		{
			AttackSpeed = FoundSpeed;
		}
	}

	// 최종 속도 계산
	FinalPlayRate = AnimSpeed * AttackSpeed;

	// 몽타주 재생 태스크 생성 
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TaskInstanceName, MontageToPlay, FinalPlayRate, NAME_None, false
	);

	if (MontageTask)
	{
		// 몽타주의 모든 종료 상황에 대해 공용 종료 함수(OnMontageCompleted) 연결
		MontageTask->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
		//MontageTask->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
		MontageTask->OnCancelled.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);

		MontageTask->ReadyForActivation();
	}

	return MontageTask;
}

const FCombatActionData& UBaseGameplayAbility::GetCombatDataFromActor()
{
	if (bIsDataCached)
	{
		return CachedCombatData;
	}

	// 처음 호출된 경우 -> 인터페이스를 통해 데이터 가져오기
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (ICombatInterface* CombatInt = Cast<ICombatInterface>(AvatarActor))
	{
		// 데이터 요청 및 저장
		CachedCombatData = CombatInt->GetCombatActionData(AbilityActionType);

		// 몽타주가 정상적으로 들어왔다면 캐싱 완료 처리
		if (CachedCombatData.MontageToPlay)
		{
			bIsDataCached = true;
			UE_LOG(LogTemp, Log, TEXT("✅ [BaseGA] 전투 데이터 캐싱 완료 (Type: %d)"), (int32)AbilityActionType);
		}
	}

	return CachedCombatData;
}

void UBaseGameplayAbility::OnMontageCompleted()
{
	// [디버깅] 몽타주가 왜 끝났는지 확인
	//UE_LOG(LogTemp, Warning, TEXT("🛑 [MeleeBase] 몽타주 종료됨! 어빌리티 End."));

	// 몽타주가 끝나면 어빌리티 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FCombatActionData UBaseGameplayAbility::GetCombatDataFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpecHandle Handle) const
{
	FCombatActionData Data; // 빈 데이터 준비

	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		// 몬스터(보스)의 스킬일 경우 -> Handle의 InputID로 인덱스를 알아내서 가져옴
		if (AbilityActionType == ECombatActionType::AIUnitSkill)
		{
			if (ASkillCasterUnit* Caster = Cast<ASkillCasterUnit>(ActorInfo->AvatarActor.Get()))
			{
				if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
				{
					if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
					{
						Data = Caster->GetSkillActionData(Spec->InputID);
					}
				}
			}
		}
		// 플레이어 평타, 무기 스킬, 일반 몹 평타
		else
		{
			if (ICombatInterface* CombatInt = Cast<ICombatInterface>(ActorInfo->AvatarActor.Get()))
			{
				Data = CombatInt->GetCombatActionData(AbilityActionType);
			}
		}
	}

	return Data;
}

//void UBaseGameplayAbility::SetJoystickLocked(const FGameplayAbilityActorInfo* ActorInfo, bool bLocked)
//{
//	if (!ActorInfo) return;
//
//	// ✅ PlayerController 대신 AvatarActor → GetController()로 접근
//	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
//	if (!AvatarActor)
//	{
//		UE_LOG(LogTemp, Error, TEXT("[JoystickLock] AvatarActor가 null입니다."));
//		return;
//	}
//
//	APawn* AvatarPawn = Cast<APawn>(AvatarActor);
//	if (!AvatarPawn)
//	{
//		UE_LOG(LogTemp, Error, TEXT("[JoystickLock] AvatarActor가 Pawn이 아닙니다."));
//		return;
//	}
//
//	AInGameController* InGamePC = Cast<AInGameController>(AvatarPawn->GetController());
//	if (!InGamePC)
//	{
//		// AI가 조종 중이면 정상적으로 null — 조용히 리턴
//		return;
//	}
//
//	UInGameHUDWidget* HUD = InGamePC->GetOrCreateInGameHUD();
//	if (!HUD) return;
//
//	UVirtualJoystickWidget* Joystick = HUD->GetVirtualJoystick();
//	if (!Joystick) return;
//
//	Joystick->SetMovementLocked(bLocked);
//	UE_LOG(LogTemp, Warning, TEXT("[JoystickLock] 조이스틱 %s 완료"), bLocked ? TEXT("잠금") : TEXT("해제"));
//}

void UBaseGameplayAbility::SetActionButtonsLocked(const FGameplayAbilityActorInfo* ActorInfo, bool bLocked)
{
	if (!ActorInfo) return;

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor) return;

	APawn* AvatarPawn = Cast<APawn>(AvatarActor);
	if (!AvatarPawn) return;

	AInGameController* InGamePC = Cast<AInGameController>(AvatarPawn->GetController());
	if (!InGamePC) return;

	UInGameHUDWidget* HUD = InGamePC->GetOrCreateInGameHUD();
	if (!HUD) return;

	// ⭐ HUD에서 ActionControlPanel을 가져온다고 가정합니다. (GetVirtualJoystick 처럼 뚫려있어야 합니다!)
	UActionControlPanel* ActionPanel = HUD->GetActionControlPanel();
	if (!ActionPanel) return;

	// 내 어빌리티 타입(AbilityActionType)을 같이 넘겨서, '내 버튼'은 잠금에서 제외하도록 지시합니다.
	ActionPanel->LockOtherActionButtons(bLocked, AbilityActionType);
}
