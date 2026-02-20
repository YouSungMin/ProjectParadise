// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/PlayerData.h"
#include "Characters/Base/CharacterBase.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/UnitStructs.h"
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

	
    CombatAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("CombatAttributeSet"));


	EquipmentComponent2 = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
}

void APlayerData::InitCombatAttributes(FCharacterStats* Stats)
{
    if (Stats)
    {
        //체력
        CombatAttributeSet->InitMaxHealth(Stats->BaseMaxHP);
        CombatAttributeSet->InitHealth(CombatAttributeSet->GetMaxHealth());
        //마나
        CombatAttributeSet->InitMaxMana(Stats->BaseMaxMP);
        CombatAttributeSet->InitMana(CombatAttributeSet->GetMaxMana());
        //공격력
        CombatAttributeSet->InitAttackPower(Stats->BaseAttackPower);
        //방어력
        CombatAttributeSet->InitDefense(Stats->BaseDefense);
        //크리티컬 확률
        CombatAttributeSet->InitCritRate(Stats->BaseCritRate);
        //이동 속도
        CombatAttributeSet->InitMoveSpeed(Stats->BaseMoveSpeed);
    }
}

void APlayerData::InitPlayerAssets(FCharacterAssets* Assets)
{
	if (Assets)
	{
		this->CachedMesh = Assets->SkeletalMesh.LoadSynchronous();
		this->CachedAnimBP = Assets->AnimBlueprint;

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
			Result.DamageMultiplier = CharStats->UltimateDamageRate;

			// 이펙트 클래스 (캐릭터 고유 이펙트가 있다면 설정)
			Result.DamageEffectClass = CharAssets->UltimateDamageEffect;
		}

		return Result; // 궁극기 데이터 반환 후 종료
	}

	// =========================================================
	// 무기 기술 (Basic Attack / Weapon Skill)
	// =========================================================

	// 1. 장비 컴포넌트 체크
	if (!EquipmentComponent2) return Result;

	// 2. 현재 무기 ID 조회
	FName WeaponID = EquipmentComponent2->GetEquippedItemID(EEquipmentSlot::Weapon);
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
		Result.AttackRange = WeaponStats->AttackRange;

		switch (ActionType)
		{
		case ECombatActionType::BasicAttack:
			Result.MontageToPlay = WeaponAssets->BasicAttackMontage.LoadSynchronous();
			Result.DamageMultiplier = 1.0f;
			break;

		case ECombatActionType::WeaponSkill:
			Result.MontageToPlay = WeaponAssets->SkillMontage.LoadSynchronous();
			Result.DamageMultiplier = WeaponStats->SkillDamageRate;
			break;
		}
	}

	return Result;
}

void APlayerData::InitializeWeaponAbilities(const FWeaponAssets* WeaponData)
{
	if (!AbilitySystemComponent || !WeaponData) return;

	UE_LOG(LogTemp, Log, TEXT("⚔️ [PlayerData] 무기 어빌리티 교체 시작..."));

	// ---------------------------------------------------------
	// 1. 기존 무기 어빌리티 제거 (Clean Up)
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
	// 2. 새 무기 어빌리티 부여 (Grant New Abilities)
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

	//스탯 데이터 조회 및 적용
	FCharacterStats* Stats = GI->GetDataTableRow<FCharacterStats>(GI->CharacterStatsDataTable, HeroID);
	if (Stats)
	{
		InitCombatAttributes(Stats);

		this->FactionTag = Stats->FactionTag;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 스탯 데이터를 찾을 수 없습니다: %s"), *HeroID.ToString());
	}

	//에셋 데이터 조회 및 적용
	FCharacterAssets* Assets = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, HeroID);
	if (Assets)
	{
		InitPlayerAssets(Assets);

		UE_LOG(LogTemp, Log, TEXT("✅ [PlayerData] 데이터 로드 완료"));
	}
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
