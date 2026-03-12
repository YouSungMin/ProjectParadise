// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/AIUnit/SkillCasterUnit.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "AbilitySystemComponent.h"
#include "Data/Assets/FXDataAsset.h"


void ASkillCasterUnit::InitializeUnit(FAIUnitStats* InStats, FAIUnitAssets* InAssets)
{
	Super::InitializeUnit(InStats, InAssets);

	// 필수 데이터가 없으면 진행 불가
	if (!InStats || !InAssets || !AbilitySystemComponent) return;

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	// 기존 캐싱 데이터 및 어빌리티 초기화
	CachedSkillDataArray.Empty();
	CachedSkillMontages.Empty();
	for (const auto& Handle : SkillAbilityHandles)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}
	SkillAbilityHandles.Empty();

	for (int32 i = 0; i < InStats->SkillActionIDs.Num(); ++i)
	{
		FCombatActionData SkillData;

		// 엑셀 데이터 로드 (사거리, 데미지 배율 등)
		if (FActionStats* ActionRow = GI->GetDataTableRow<FActionStats>(GI->ActionStatsDataTable, InStats->SkillActionIDs[i]))
		{
			SkillData.Stats = *ActionRow;
		}

		// 스킬 셋업 (이펙트, 투사체 포장 & GAS 어빌리티 부여)
		if (InAssets->SkillSetups.IsValidIndex(i))
		{
			SkillData.EffectClass = InAssets->SkillSetups[i].EffectClass;
			SkillData.ProjectileClass = InAssets->SkillSetups[i].ProjectileClass;

			// 인덱스(i)를 InputID로 사용하여 어빌리티 즉시 부여
			if (TSubclassOf<UGameplayAbility> SkillClass = InAssets->SkillSetups[i].AbilityClass)
			{
				FGameplayAbilitySpec Spec(SkillClass, 1, i);
				SkillAbilityHandles.Add(AbilitySystemComponent->GiveAbility(Spec));
				UE_LOG(LogTemp,Log,TEXT("ASkillCasterUnit %d 번째 패턴 어빌리티 부여 성공 "),i);
			}
		}

		// 몽타주 포장 및 캐싱
		if (InAssets->SkillMontages.IsValidIndex(i))
		{
			UAnimMontage* LoadedMontage = InAssets->SkillMontages[i].LoadSynchronous();
			SkillData.MontageToPlay = LoadedMontage;
			CachedSkillMontages.Add(LoadedMontage); // (기존 호환성 유지용)
		}
		else
		{
			CachedSkillMontages.Add(nullptr); // 인덱스 꼬임 방지
		}

		// 완성된 종합 선물 세트를 배열에 쏙!
		CachedSkillDataArray.Add(SkillData);
	}

	// 적군(Enemy)일 경우 연출 태그 캐싱
	if (FactionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Unit.Faction.Enemy")))
	{
		if (FEnemyAssets* EnemyAssets = static_cast<FEnemyAssets*>(InAssets))
		{
			CachedSkillEffectTags = EnemyAssets->SkillEffectTags;
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