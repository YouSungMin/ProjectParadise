// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/PlayerData.h"
#include "Characters/Base/CharacterBase.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/InventorySystem.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/GrowthStruct.h"
#include "Data/Assets/FXDataAsset.h"
#include "AbilitySystemComponent.h"
#include "Components/EquipmentComponent.h"
#include "Data/Enums/GameEnums.h"

APlayerData::APlayerData()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = false; 

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(false);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	
    CombatAttributeSet2 = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("CombatAttributeSet"));


	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
}

void APlayerData::InitCombatAttributes()
{
	if (!CombatAttributeSet2) {
		UE_LOG(LogTemp, Log, TEXT("✅ [PlayerData] CombatAttributeSet가 없습니다."));
		return;
	} 

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;


	//캐릭터 기본 스탯 로드
	FCharacterStats* Stats = GI->GetDataTableRow<FCharacterStats>(GI->CharacterStatsDataTable, CharacterID);
	if (!Stats) return;

	// 캐릭터 레벨 및 각성 데이터 로드
	float BonusLevelHP = 0.0f;
	float BonusLevelAttack = 0.0f;
	float BonusLevelDefense = 0.0f;
	float AwakenMultiplier = 1.0f;

	// 레벨업 데이터 테이블 조회
	FName LevelRowName = FName(*FString::FromInt(CurrentLevel));
	if (FCharacterLevelUpData* LevelData = GI->GetDataTableRow<FCharacterLevelUpData>(GI->CharacterLevelUpDataTable, LevelRowName))
	{
		BonusLevelHP = LevelData->BonusMaxHP;
		BonusLevelAttack = LevelData->BonusAttackPower;
		BonusLevelDefense = LevelData->BonusDefense;
	}

	// 각성(돌파) 데이터 테이블 조회
	FName AwakenRowName = FName(*FString::FromInt(CurrentAwakenLevel));
	if (FCharacterAwakenData* AwakenData = GI->GetDataTableRow<FCharacterAwakenData>(GI->CharacterAwakenDataTable, AwakenRowName))
	{
		AwakenMultiplier = AwakenData->BonusStatMultiplier;
	}

	// 캐릭터 순수 스탯 최종 계산
	float CharMaxHP = (Stats->BaseMaxHP + BonusLevelHP) * AwakenMultiplier;
	float CharMaxMP = Stats->BaseMaxMP;
	float CharAttack = (Stats->BaseAttackPower + BonusLevelAttack) * AwakenMultiplier;
	float CharDefense = (Stats->BaseDefense + BonusLevelDefense) * AwakenMultiplier;
	float CharCritRate = Stats->BaseCritRate;
	float CharMoveSpeed = Stats->BaseMoveSpeed;


	// 장비 스탯 합산 (강화 수치 연동)
	float EquipMaxHP = 0.0f;
	float EquipMaxMP = 0.0f;
	float EquipAttack = 0.0f;
	float EquipDefense = 0.0f;
	float EquipCritRate = 0.0f;

	// 추가 스탯들
	float EquipCritDamage = 0.0f;
	float EquipAttackSpeed = 0.0f;
	float FinalAttackRange = 0.0f;

	if (EquipmentComponent)
	{
		for (const auto& Pair : EquipmentComponent->GetEquippedItems())
		{
			EEquipmentSlot Slot = Pair.Key;
			FOwnedItemData ItemData;

			if (EquipmentComponent->GetEquippedItemData(Slot, ItemData))
			{
				bool bIsWeapon = (Slot == EEquipmentSlot::Weapon);
				FName TargetCostID = NAME_None; // LevelUpCostId를 저장할 변수
				FWeaponStats* WeaponStats = nullptr;
				FArmorStats* ArmorStats = nullptr;

				//장비 분류에 따른 원본 스탯 및 LevelUpCostId 가져오기
				if (bIsWeapon)
				{
					WeaponStats = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, ItemData.ItemID);
					if (WeaponStats) TargetCostID = WeaponStats->LevelUpCostId;
				}
				else
				{
					ArmorStats = GI->GetDataTableRow<FArmorStats>(GI->ArmorStatsDataTable, ItemData.ItemID);
					if (ArmorStats) TargetCostID = ArmorStats->LevelUpCostId;
				}

				//강화 배율 계산 (기본값 1.0f)
				float EnhanceMult = 1.0f;

				// 강화 레벨이 1 이상이고, 강화 비용 ID가 유효할 때만 테이블 조회
				if (ItemData.EnhancementLevel > 0 && !TargetCostID.IsNone())
				{
					// 문자열 변환 없이 바로 TargetCostID를 키값으로 사용
					if (FEquipmentEnhanceData* EnhanceData = GI->GetDataTableRow<FEquipmentEnhanceData>(GI->EquipmentEnhanceDataTable, TargetCostID))
					{
						// 공식: 1.0 + (레벨당 증가치 * 강화 레벨)
						// 예: 0.1 증가치인 무기를 5강하면 -> 1.0 + (0.1 * 5) = 1.5배 적용
						EnhanceMult = 1.0f + (EnhanceData->StatBonusPerLevel * ItemData.EnhancementLevel);
					}
				}

				//무기 스탯 계산 및 누적
				if (bIsWeapon && WeaponStats)
				{
					// 공격력에는 강화 배율 적용
					EquipAttack += (WeaponStats->AttackPower * EnhanceMult);

					// 공속, 치명타 등은 강화 배율 적용 제외 (원하신다면 추가 가능)
					EquipCritRate += WeaponStats->CritRate;
					EquipCritDamage += WeaponStats->CritDamage;
					EquipAttackSpeed += WeaponStats->AttackSpeed;

					// 사거리 스탯 추출
					if (!WeaponStats->BasicAttackActionID.IsNone())
					{
						if (FActionStats* ActionRow = GI->GetDataTableRow<FActionStats>(GI->ActionStatsDataTable, WeaponStats->BasicAttackActionID))
						{
							FinalAttackRange = ActionRow->AttackRange;
						}
					}
				}
				//방어구 스탯 계산 및 누적
				else if (!bIsWeapon && ArmorStats)
				{
					// 방어구 관련 스탯들에 강화 배율 적용
					EquipMaxHP += (ArmorStats->MaxHP * EnhanceMult);
					EquipMaxMP += (ArmorStats->MaxMana * EnhanceMult);
					EquipDefense += (ArmorStats->DefensePower * EnhanceMult);
				}
			}
		}
	}

	//최종 스탯 GAS AttributeSet에 적용
	float FinalMaxHP = CharMaxHP + EquipMaxHP;
	float FinalMaxMP = CharMaxMP + EquipMaxMP;
	float FinalAttack = CharAttack + EquipAttack;
	float FinalDefense = CharDefense + EquipDefense;
	float FinalCritRate = CharCritRate + EquipCritRate;

	CombatAttributeSet2->InitMaxHealth(FinalMaxHP);
	CombatAttributeSet2->InitHealth(FinalMaxHP); // 현재 체력도 최대로 갱신
	CombatAttributeSet2->InitMaxMana(FinalMaxMP);
	CombatAttributeSet2->InitMana(FinalMaxMP);   // 현재 마나도 최대로 갱신
	CombatAttributeSet2->InitAttackPower(FinalAttack);
	CombatAttributeSet2->InitDefense(FinalDefense);
	CombatAttributeSet2->InitCritRate(FinalCritRate);
	CombatAttributeSet2->InitMoveSpeed(CharMoveSpeed);

	CombatAttributeSet2->InitCritDamage(EquipCritDamage); // 캐릭터 기본 크뎀이 있다면 CharCritDamage + EquipCritDamage로 합산 필요
	CombatAttributeSet2->InitAttackSpeed(EquipAttackSpeed);

	if (FinalAttackRange > 0.0f)
	{
		CombatAttributeSet2->InitAttackRange(FinalAttackRange);
	}

	UE_LOG(LogTemp, Log, TEXT("✅ [PlayerData] 스탯 초기화 완료 (Level: %d, HP: %.1f, Attack: %.1f)"), CurrentLevel, FinalMaxHP, FinalAttack);

}

void APlayerData::InitPlayerAssets()
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	//데이터 조회후 초기화
	FCharacterAssets* Assets = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, CharacterID);
	if (!Assets) return;

	this->CachedMesh = Assets->SkeletalMesh.LoadSynchronous();
	this->CachedDeathMontage = Assets->DeathMontage.LoadSynchronous();
	this->CachedHitMontage = Assets->HitMontage.LoadSynchronous();
	this->CachedAnimBP = Assets->AnimBlueprint;
	this->FactionTag = Assets->FactionTag;
	this->CachedReactionFX = Assets->ReactionFX;
	this->CachedUltimateFXTag = Assets->UltimateEffectTag;

	//ASC 세팅
	if (AbilitySystemComponent)
	{
		// 기존 궁극기가 있다면 제거 (재초기화/리스폰 대비)
		if (UltimateSkillHandle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(UltimateSkillHandle);
			UltimateSkillHandle = FGameplayAbilitySpecHandle(); // 초기화
		}

		// 새 궁극기 부여
		if (Assets->UltimateAbility)
		{
			// InputID는 프로젝트 설정에 맞게 변경 (예: Skill_Ultimate or 3, 4번 등)
			FGameplayAbilitySpec Spec(Assets->UltimateAbility, 1, static_cast<int32>(EInputID::Ultimate));

			UltimateSkillHandle = AbilitySystemComponent->GiveAbility(Spec);

			UE_LOG(LogTemp, Log, TEXT("✅ [PlayerData] 궁극기(Ultimate) 어빌리티 부여 완료"));
		}
	}

	//장비
	if (EquipmentComponent)
	{
		FName WeaponID = EquipmentComponent->GetEquippedItemID(EEquipmentSlot::Weapon);

		if (!WeaponID.IsNone())
		{
			// 무기 테이블 조회
			FWeaponAssets* WeaponAssets = GI->GetDataTableRow<FWeaponAssets>(GI->WeaponAssetsDataTable, WeaponID);
			if (WeaponAssets)
			{
				// 스킬 및 애니메이션 부여
				this->InitializeWeaponAbilities(WeaponAssets);
				UE_LOG(LogTemp, Log, TEXT("⚔️ [PlayerData] 무기(%s) 어빌리티가 성공적으로 부여되었습니다."), *WeaponID.ToString());
			}
		}
	}
	

}

FCombatActionData APlayerData::GetCombatActionData(ECombatActionType ActionType) const
{
	FCombatActionData Result;

	// 1. GameInstance 가져오기 (필수)
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [PlayerData] GameInstance 없음!"));
		return Result;
	}

	// =========================================================
	// 궁극기 (Ultimate Skill) - 캐릭터 고유 능력
	// =========================================================
	if (ActionType == ECombatActionType::UltimateSkill)
	{
		// 내 캐릭터 ID로 에셋 테이블 조회
		FCharacterAssets* CharAssets = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, CharacterID);
		FCharacterStats* CharStats = GI->GetDataTableRow<FCharacterStats>(GI->CharacterStatsDataTable, CharacterID);

		if (CharAssets)
		{
			Result.MontageToPlay = CharAssets->UltimateMontage.LoadSynchronous(); // 구조체에 이 필드가 있다고 가정
			if (FActionStats* ActionRow = GI->GetDataTableRow<FActionStats>(GI->ActionStatsDataTable, CharStats->SkillActionID))
			{
				Result.DamageMultiplier = ActionRow->DamageMultiplier;
				Result.AttackRange = ActionRow->AttackRange;
				Result.AttackRadius = ActionRow->AttackRadius;
				Result.ForwardOffset = ActionRow->ForwardOffset;
				Result.ProjectileSpeed = ActionRow->ProjectileSpeed;
			}

			// 이펙트 클래스 (캐릭터 고유 이펙트가 있다면 설정)
			Result.DamageEffectClass = CharAssets->UltimateDamageEffect;
		}

		return Result; // 궁극기 데이터 반환 후 종료
	}

	// =========================================================
	// 무기 기술 (Basic Attack / Weapon Skill)
	// =========================================================

	// 1. 장비 컴포넌트 체크
	if (!EquipmentComponent) return Result;

	// 2. 현재 무기 ID 조회
	FName WeaponID = EquipmentComponent->GetEquippedItemID(EEquipmentSlot::Weapon);
	if (WeaponID.IsNone()) return Result;

	// 3. 무기 데이터 테이블 조회
	FWeaponAssets* WeaponAssets = GI->GetDataTableRow<FWeaponAssets>(GI->WeaponAssetsDataTable, WeaponID);
	FWeaponStats* WeaponStats = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, WeaponID);

	// 4. 데이터 패키징
	if (WeaponAssets && WeaponStats)
	{
		// 공통: 무기 전용 데미지 이펙트 (독, 화염 등)
		Result.DamageEffectClass = WeaponAssets->DamageEffectClass;
		Result.ProjectileClass = WeaponAssets->ProjectileClass;
		FName TargetActionID = NAME_None;

		FCharacterAssets* CharAssets = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, CharacterID);

		if (CharAssets && CharAssets->WeaponAnimMap.Contains(WeaponAssets->WeaponType))
		{
			FWeaponAnimSet MyAnimSet = CharAssets->WeaponAnimMap[WeaponAssets->WeaponType];
			switch (ActionType)
			{
			case ECombatActionType::BasicAttack:
				Result.MontageToPlay = MyAnimSet.BasicAttackMontage.LoadSynchronous();
				TargetActionID = WeaponStats->BasicAttackActionID;
				break;

			case ECombatActionType::WeaponSkill:
				Result.MontageToPlay = MyAnimSet.SkillMontage.LoadSynchronous();
				TargetActionID = WeaponStats->SkillActionID;
				break;
			}

			if (!TargetActionID.IsNone())
			{
				if (FActionStats* ActionRow = GI->GetDataTableRow<FActionStats>(GI->ActionStatsDataTable, TargetActionID))
				{
					Result.DamageMultiplier = ActionRow->DamageMultiplier;
					Result.AttackRange = ActionRow->AttackRange;
					Result.AttackRadius = ActionRow->AttackRadius;
					Result.ForwardOffset = ActionRow->ForwardOffset;
					Result.Cooldown = ActionRow->Cooldown;
					Result.ProjectileSpeed = ActionRow->ProjectileSpeed;
					Result.ManaCost = ActionRow->ManaCost;
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("❌ [PlayerData] 엑셀에서 ActionID(%s)를 찾을 수 없습니다!"), *TargetActionID.ToString());
				}
			}
		}
	}

	return Result;
}

void APlayerData::InitializeWeaponAbilities(const FWeaponAssets* WeaponData)
{
	if (!AbilitySystemComponent || !WeaponData) return;

	UE_LOG(LogTemp, Log, TEXT("⚔️ [PlayerData] 무기 어빌리티 교체 시작..."));

	// ---------------------------------------------------------
	// 기존 무기 어빌리티 제거 (Clean Up)
	// ---------------------------------------------------------
	if (BasicAttackHandle.IsValid())
	{
		AbilitySystemComponent->ClearAbility(BasicAttackHandle);
		BasicAttackHandle = FGameplayAbilitySpecHandle();
	}

	if (WeaponSkillHandle.IsValid())
	{
		AbilitySystemComponent->ClearAbility(WeaponSkillHandle);
		WeaponSkillHandle = FGameplayAbilitySpecHandle();
	}

	// ---------------------------------------------------------
	// 새 무기 어빌리티 부여 (Grant New Abilities)
	// ---------------------------------------------------------

	// 평타 (Basic Attack)
	if (WeaponData->BasicAttackAbility)
	{
		FGameplayAbilitySpec Spec(WeaponData->BasicAttackAbility, 1, static_cast<int32>(EInputID::Attack));
		BasicAttackHandle = AbilitySystemComponent->GiveAbility(Spec);
	}

	// 무기 스킬 (Weapon Skill)
	if (WeaponData->WeaponSkillAbility)
	{
		FGameplayAbilitySpec Spec(WeaponData->WeaponSkillAbility, 1, static_cast<int32>(EInputID::Skill));
		WeaponSkillHandle = AbilitySystemComponent->GiveAbility(Spec);
	}

	UE_LOG(LogTemp, Log, TEXT("✅ [PlayerData] 무기 어빌리티 부여 완료 (평타/스킬)"));

	// ---------------------------------------------------------
	// 무기 FX 데이터 캐싱
	// ---------------------------------------------------------
	if (WeaponData)
	{
		CachedActionFX = WeaponData->ActionFX;
		UE_LOG(LogTemp, Log, TEXT("✅ [PlayerData] 무기 FX 데이터 캐싱 완료"));
	}
	else
	{
		// 무기를 해제했을 경우 비워줌
		CachedActionFX = FActionFXSettings();
	}
}



void APlayerData::InitPlayerData(FName HeroID)
{
	
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [PlayerData] GameInstance를 찾을 수 없습니다."));
		return;
	}
	this->CharacterID = HeroID;
	UE_LOG(LogTemp, Log, TEXT("🔄 [PlayerData] 영웅 초기화 시작: %s"), *HeroID.ToString());

	UInventorySystem* InvSys = GI->GetMainInventory();
	if (InvSys)
	{
		const FOwnedCharacterData* MyData;
		bool bFound = false;
		MyData = InvSys->GetCharacterDataByID(HeroID);// 함수명은 실제 구현에 맞게 변경하세요.

		if (MyData) {
			bFound = true;
		}

		if (bFound)
		{
			// 인벤토리의 진짜 성장 데이터를 내 변수에 복사
			this->CurrentLevel = MyData->Level;
			this->CurrentAwakenLevel = MyData->AwakeningLevel;

			// 인벤토리에 저장된 진짜 장착 장비 맵을 장비 컴포넌트에 넘겨줌!
			if (EquipmentComponent)
			{
				EquipmentComponent->InitializeEquipment(MyData->EquipmentMap);
			}
			UE_LOG(LogTemp, Log, TEXT("✅ [PlayerData] 인벤토리 데이터 연동 완료 (Lv.%d)"), CurrentLevel);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [PlayerData] 인벤토리에서 %s 데이터를 못 찾음. 1레벨 기본값 적용."), *HeroID.ToString());
		}
	}

	//전투 스탯 초기화 (내부에서 스스로 테이블 조회)
	InitCombatAttributes();

	//시각적 에셋 초기화 (내부에서 스스로 테이블 조회)
	InitPlayerAssets();

	UE_LOG(LogTemp, Log, TEXT("✅ [PlayerData] 데이터 로드 및 초기화 완료"));
	
}

void APlayerData::OnDeath()
{
	if (bIsDead) return;
	bIsDead = true;

	// 부활 타이머 시작 (예: 5초 뒤 부활)
    UE_LOG(LogTemp, Error, TEXT("👻 [PlayerData] 영혼 사망 확인. 5초 뒤 리스폰 가능합니다"));
	GetWorld()->GetTimerManager().SetTimer(
		RespawnTimerHandle, 
		this, 
		&APlayerData::OnRespawnFinished, 
		RespawnTimer, 
		false);

	UE_LOG(LogTemp, Warning, TEXT("5초 뒤 부활 예정."));
    // TODO: 여기서 GameMode나 PlayerController에게 "새 몸 줘!"라고 요청하는 코드 필요
    // 예: GetWorld()->GetAuthGameMode<AMyGameMode>()->RespawnHero(this);
}

void APlayerData::OnRespawnFinished()
{
	bIsDead = false;
	UE_LOG(LogTemp, Warning, TEXT("부활 완료! 재생성 가능."));
}

FFXPayload* APlayerData::GetFXPayload(EFXEventType EventType) const
{
	UFXDataAsset* TargetAsset = nullptr;
	FGameplayTag TargetTag;

	// 요청 타입(Enum)에 따라 알맞은 에셋과 태그를 매핑 (라우팅)
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
		TargetAsset = CachedActionFX.ActionFXData.LoadSynchronous();
		TargetTag = CachedActionFX.SkillTag;
		break;
	case EFXEventType::Ultimate:
		TargetAsset = CachedReactionFX.ReactionFXData.LoadSynchronous();
		TargetTag = CachedUltimateFXTag;
		break;
	}

	// 에셋이 있고, 태그가 유효하다면 직접 검색해서 보따리(Payload) 반환
	if (TargetAsset && TargetTag.IsValid())
	{
		return TargetAsset->FindEffect(TargetTag);
	}

	return nullptr;
}