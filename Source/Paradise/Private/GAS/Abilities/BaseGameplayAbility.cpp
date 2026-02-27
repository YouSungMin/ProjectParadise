// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/BaseGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "Kismet/GameplayStatics.h"

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

	// Spec 생성
	return SourceASC->MakeOutgoingSpec(EffectClass, Level, Context);
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

		FCombatActionData CombatData = const_cast<UBaseGameplayAbility*>(this)->GetCombatDataFromActor();

		UE_LOG(LogTemp, Warning, TEXT("⏳ [ApplyCooldown] 적용 시도! 엑셀 쿨타임: %.1f"), CombatData.Cooldown);

		if (CombatData.Cooldown > 0.0f)
		{
			FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(FName("Data.Cooldown"));
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(CooldownTag, CombatData.Cooldown);
		}

		// 나 자신에게 쿨타임 이펙트 적용
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [ApplyCooldown] 어빌리티에 Cooldown GE가 설정되지 않았습니다!"));
	}
}

bool UBaseGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	// 엑셀에서 읽어둔 내 스킬 데이터 가져오기
	FCombatActionData CombatData = const_cast<UBaseGameplayAbility*>(this)->GetCombatDataFromActor();

	// 소모 마나가 없으면(0이면) 무조건 패스 (평타 등)
	if (CombatData.ManaCost <= 0.0f)
	{
		return true;
	}

	// 현재 내 마나량과 비교
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		float CurrentMana = ASC->GetNumericAttribute(UBaseAttributeSet::GetManaAttribute());

		if (CurrentMana < CombatData.ManaCost)
		{
			UE_LOG(LogTemp, Warning, TEXT("❌ 마나가 부족합니다! (현재: %.1f / 필요: %.1f)"), CurrentMana, CombatData.ManaCost);
			return false; // 발동 실패!
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
		FCombatActionData CombatData = const_cast<UBaseGameplayAbility*>(this)->GetCombatDataFromActor();

		if (CombatData.ManaCost > 0.0f)
		{
			// SetByCaller를 통해 엑셀의 마나 코스트 수치를 GE로 전송
			FGameplayTag CostTag = FGameplayTag::RequestGameplayTag(FName("Data.Cost.Mana"));

			SpecHandle.Data.Get()->SetSetByCallerMagnitude(CostTag, -CombatData.ManaCost);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			UE_LOG(LogTemp,Log,TEXT("ApplyCost %.1f"), -CombatData.ManaCost);
		}
	}
	else
	{
		// 마나가 드는 스킬인데 블루프린트에서 Cost GE를 빼먹은 경우 에러 로그 출력
		FCombatActionData CombatData = const_cast<UBaseGameplayAbility*>(this)->GetCombatDataFromActor();
		if (CombatData.ManaCost > 0.0f)
		{
			UE_LOG(LogTemp, Error, TEXT("❌ [ApplyCost] 엑셀에 마나 소모량이 기입되었으나, 블루프린트의 Cost GE Class가 설정되지 않았습니다!"));
		}
	}
}

UAbilityTask_PlayMontageAndWait* UBaseGameplayAbility::PlayMontageAndWaitCallback(UAnimMontage* MontageToPlay, FName TaskInstanceName)
{
	if (!MontageToPlay) return nullptr;

	// 몽타주 재생 태스크 생성 
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TaskInstanceName, MontageToPlay, 1.0f, NAME_None, false
	);

	if (MontageTask)
	{
		// 몽타주의 모든 종료 상황에 대해 공용 종료 함수(OnMontageCompleted) 연결
		MontageTask->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
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
