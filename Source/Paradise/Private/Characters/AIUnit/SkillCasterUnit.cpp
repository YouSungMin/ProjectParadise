// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/AIUnit/SkillCasterUnit.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "AbilitySystemComponent.h"
#include "Data/Assets/FXDataAsset.h"


void ASkillCasterUnit::InitializeUnit(FAIUnitStats* InStats, FAIUnitAssets* InAssets)
{
	// 부모의 초기화 로직 실행
	Super::InitializeUnit(InStats, InAssets);

	// 다중 스킬 스탯 캐싱 (InStats->SkillActionIDs 순회)
	if (InStats)
	{
		CachedSkillDataArray.Empty();
		if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance()))
		{
			for (const FName& SkillID : InStats->SkillActionIDs)
			{
				FCombatActionData SkillData;
				if (FActionStats* ActionRow = GI->GetDataTableRow<FActionStats>(GI->ActionStatsDataTable, SkillID))
				{
					SkillData.AttackRange = ActionRow->AttackRange;
					SkillData.DamageMultiplier = ActionRow->DamageMultiplier;
					SkillData.AttackRadius = ActionRow->AttackRadius;
					SkillData.ForwardOffset = ActionRow->ForwardOffset;
				}
				CachedSkillDataArray.Add(SkillData);
			}
		}
	}

	if (InAssets)
	{
		// 다중 스킬 어빌리티(GAS) 완벽한 재설정 및 부여
		if (AbilitySystemComponent)
		{
			// 기존 스킬 초기화 및 비우기
			for (const auto& Handle : SkillAbilityHandles)
			{
				AbilitySystemComponent->ClearAbility(Handle);
			}
			SkillAbilityHandles.Empty();

			// 새 스킬 부여 및 디버그 로그 출력
			for (int32 i = 0; i < InAssets->SkillAbilities.Num(); ++i)
			{
				TSubclassOf<UGameplayAbility> SkillClass = InAssets->SkillAbilities[i];
				if (SkillClass)
				{
					FGameplayAbilitySpec Spec(SkillClass, 1, i);
					FGameplayAbilitySpecHandle GivenHandle = AbilitySystemComponent->GiveAbility(Spec);

					// 디버그 로그: 핸들이 유효한지(부여 성공) 확인
					if (GivenHandle.IsValid())
					{
						SkillAbilityHandles.Add(GivenHandle);
						UE_LOG(LogTemp, Log, TEXT("🟢 [%s] 스킬 부여 성공! | 어빌리티: %s | 할당된 인덱스(InputID): %d"),
							*GetName(), *SkillClass->GetName(), i);
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("❌ [%s] 스킬 부여 실패! | 어빌리티: %s | 인덱스: %d"),
							*GetName(), *SkillClass->GetName(), i);
					}
				}
				else
				{
					// 배열 칸은 있는데 BP를 안 채워 넣었을 경우의 경고 로그
					UE_LOG(LogTemp, Warning, TEXT("⚠️ [%s] %d번 인덱스의 스킬 클래스가 비어있습니다! (데이터 테이블 확인 요망)"),
						*GetName(), i);
				}
			}
		}

		// 스킬 연출 태그 캐싱
		if (FactionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Unit.Faction.Enemy")))
		{
			if (FEnemyAssets* EnemyAssets = static_cast<FEnemyAssets*>(InAssets))
			{
				CachedSkillEffectTags = EnemyAssets->SkillEffectTags;
			}
		}

		// 스킬 몽타주 캐싱
		CachedSkillMontages.Empty();
		for (const TSoftObjectPtr<UAnimMontage>& SoftMontage : InAssets->SkillMontages)
		{
			if (!SoftMontage.IsNull())
			{
				CachedSkillMontages.Add(SoftMontage.LoadSynchronous());
			}
			else
			{
				CachedSkillMontages.Add(nullptr); // 인덱스 동기화
			}
		}
	}
}

FCombatActionData ASkillCasterUnit::GetSkillActionData(int32 SkillIndex) const
{
	if (CachedSkillDataArray.IsValidIndex(SkillIndex))
	{
		return CachedSkillDataArray[SkillIndex];
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] 잘못된 스킬 인덱스 요청: %d"), *GetName(), SkillIndex);
	return FCombatActionData();
}

FFXPayload* ASkillCasterUnit::GetSkillFXPayload(int32 SkillIndex) const
{
	if (CachedSkillEffectTags.IsValidIndex(SkillIndex))
	{
		FGameplayTag TargetTag = CachedSkillEffectTags[SkillIndex];

		// 부모 클래스(AUnitBase)에 있는 CachedActionFX.ActionFXData 활용
		if (CachedActionFX.ActionFXData && TargetTag.IsValid())
		{
			UFXDataAsset* LoadedAsset = CachedActionFX.ActionFXData.LoadSynchronous();
			if (LoadedAsset)
			{
				return LoadedAsset->FindEffect(TargetTag);
			}
		}
	}
	return nullptr;
}

UAnimMontage* ASkillCasterUnit::GetSkillMontage(int32 SkillIndex) const
{
	if (CachedSkillMontages.IsValidIndex(SkillIndex))
	{
		return CachedSkillMontages[SkillIndex];
	}

	UE_LOG(LogTemp, Warning, TEXT("❌ [%s] 잘못된 스킬 몽타주 인덱스 요청: %d"), *GetName(), SkillIndex);
	return nullptr;
}

FCombatActionData ASkillCasterUnit::GetCombatActionData(ECombatActionType ActionType) const
{
	// 노티파이가 '스킬 데이터'를 달라고 했고, 현재 내가 캐스팅 중인 스킬이 있다면?
	if (ActionType == ECombatActionType::AIUnitSkill && CurrentCastingSkillIndex != INDEX_NONE)
	{
		// 아까 만들어둔 인덱스 전용 Getter를 호출해서 반환!
		return GetSkillActionData(CurrentCastingSkillIndex);
	}

	return Super::GetCombatActionData(ActionType);
}

FFXPayload* ASkillCasterUnit::GetFXPayload(EFXEventType EventType) const
{
	if (EventType == EFXEventType::Skill && CurrentCastingSkillIndex != INDEX_NONE)
	{
		return GetSkillFXPayload(CurrentCastingSkillIndex);
	}

	return Super::GetFXPayload(EventType);
}