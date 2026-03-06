#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Data/Enums/GameEnums.h"
#include "GameplayEffect.h"
#include "Data/Structs/FXStructs.h"
#include "ItemStructs.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UTexture2D;
class UGameplayAbility;
class UAnimMontage;
class USoundBase;
class UNiagaraSystem;
class UFXDataAsset;

/**
 * @struct FItemBaseStats
 * @brief 아이템의 수치(Data)와 로직적인 속성을 담당하는 최상위 부모 구조체입니다.
 * @details 모든 아이템(무기, 방어구 등)이 공통적으로 가지는 이름, 등급, 설명 등을 포함합니다.
 */
USTRUCT(BlueprintType)
struct FItemBaseStats : public FTableRowBase
{
	GENERATED_BODY()

public:

	/**
	 * @brief 인게임 UI에 표시될 아이템의 이름
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Info")
	FText DisplayName;

	/**
	 * @brief 아이템 설명 텍스트
	 * @details 툴팁이나 상세 정보창에 표시됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Info", meta = (MultiLine = true))
	FText Description;

	/**
	 * @brief 아이템 희귀도 (Rarity)
	 * @details Common ~ Legendary 드랍 확률 및 UI 테두리 색상 결정에 사용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Info")
	EItemRarity Rarity;

	/**
	 * @brief 세트 효과 ID (Set Bonus ID)
	 * @details DT_SetBonus 테이블의 RowName과 일치해야 합니다. (세트 효과가 없으면 None)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Info")
	FName SetID;

	/**
	 * @brief 레벨업 비용 ID (Upgrade Cost ID)
	 * @details 아이템 레벨 업 시 필요한 재화 정보를 담고 있는 테이블의 RowName입니다.
	 * DT_LevelUpCost 테이블을 참조합니다. (None일 경우 강화 불가)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FName LevelUpCostId;
};

/**
 * @struct FWeaponStats
 * @brief 무기 전용 수치 데이터 구조체
 * @details FItemBaseStats를 상속받아 공격력, 사거리 등 전투에 필요한 구체적인 스탯을 정의합니다.
 */
USTRUCT(BlueprintType)
struct FWeaponStats : public FItemBaseStats
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------
	// [Combat Stats] 전투 관련 수치
	// -----------------------------------------------------------------

	/**
	 * @brief 기본 공격력 (Base Damage)
	 * @details 무기의 순수 데미지 값입니다.
	 * GameplayEffectExecutionCalculation(ExecCalc)에서 캐릭터의 스탯(Strength)과 합산되어 최종 데미지가 계산됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats", meta = (ClampMin = "0.0"))
	float AttackPower;

	/**
	 * @brief 공격 속도 배율 (Attack Speed Multiplier)
	 * @details 기본값 1.0을 기준으로, 높을수록 공격 속도가 빨라집니다.
	 * 예: 1.2 = 20% 더 빠름. 애니메이션 재생 속도(PlayRate)에 곱해지거나 쿨타임 감소에 영향을 줍니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float AttackSpeed;

	/**
	 * @brief 치명타 확률 (Critical Rate)
	 * @details 0.0 ~ 1.0 사이의 정규화된 값을 사용합니다.
	 * 예: 0.15 = 15% 확률
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CritRate;

	/**
	 * @brief 치명타 피해량 배율 (Critical Damage Multiplier)
	 * @details 치명타 발생 시 기본 데미지에 곱해지는 배율입니다.
	 * 예: 1.5 = 150% 데미지 (기본 100 + 추가 50). 2.0 = 200% 데미지.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats", meta = (ClampMin = "1.0"))
	float CritDamage;

	/**
	 * @brief 평타 - FActionStats 테이블의 ID로 사용
	 * @details 개별 액션의 수치를 정의 해둔 구조체의 RowName
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionLink")
	FName BasicAttackActionID; // 예: "Sword_BasicCombo"

	/**
	 * @brief 스킬 - FActionStats 테이블의 ID로 사용
	 * @details 개별 액션의 수치를 정의 해둔 구조체의 RowName
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionLink")
	FName SkillActionID;
};

/**
 * @struct FArmorStats
 * @brief 방어구 전용 수치 데이터 구조체
 * @details FItemBaseStats를 상속받아 최대 체력, 최대 마나, 방어력 등 생존과 관련된 구체적인 스탯을 정의합니다.
 */
USTRUCT(BlueprintType)
struct FArmorStats : public FItemBaseStats
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------
	// [Survival Stats] 방어 관련 수치 
	// -----------------------------------------------------------------

	/**
	 * @brief 방어력 (DefensePower)
	 * @details 적의 공격 데미지를 감소시키는 수치입니다.
	 * GameplayEffectExecutionCalculation(ExecCalc)에서 데미지 공식의 감산(Subtraction) 혹은 비율 감소 인자로 활용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival Stats")
	float DefensePower;

	/**
	 * @brief 최대 체력 보너스 (Bonus Max HP)
	 * @details 장착 시 캐릭터의 최대 체력(MaxHealth)을 증가시키는 절대값(Additive)입니다.
	 * 예: 100 입력 시 MaxHealth가 100 증가. GAS의 Passive GameplayEffect를 통해 적용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival Stats")
	float MaxHP;

	/**
	 * @brief 최대 마나 보너스 (Bonus Max Mana)
	 * @details 장착 시 캐릭터의 최대 마나(MaxMana)를 증가시키는 절대값(Additive)입니다.
	 * 스킬 사용 빈도를 높여주는 옵션으로 사용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival Stats")
	float MaxMana;
};

/**
 * @struct FItemBaseAssets
 * @brief 아이템의 외형(Visual)과 UI 리소스를 관리하는 부모 구조체
 * @details 외형(Mesh), 기본 애니메이션(AnimBP), 생존 반응(Hit/Dead) 등 필수적인 에셋을 포함합니다.
 */
USTRUCT(BlueprintType)
struct FItemBaseAssets : public FTableRowBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief 아이템의 3D 모델링 (Skeletal Mesh)
	 * @details 무기는 손에 쥐어지는 메쉬, 방어구는 캐릭터 몸을 대체하거나 덧입혀지는 메쉬입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> ItemMesh;

	/**
	 * @brief 부착 소켓 이름 (Attachment Socket)
	 * @details 아이템이 캐릭터의 어느 소켓에 붙을지 결정합니다. (예: "Weapon_R", "Spine_01")
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	FName AttachmentSocket;

	/**
	 * @brief UI 아이콘 (Inventory Icon)
	 * @details 인벤토리, 상점, 장비창 등에서 보여줄 2D 텍스처입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * @struct FWeaponAssets
 * @brief 무기 전용 에셋 데이터
 * @details 공격 모션, 타격음, 이펙트, GAS 어빌리티 등 전투 리소스를 포함합니다.
 */
USTRUCT(BlueprintType)
struct FWeaponAssets : public FItemBaseAssets
{
	GENERATED_BODY()

public:
	/**
	 * @brief 인게임 무기 평타 UI 아이콘
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftObjectPtr<UTexture2D> WeaponBasicAttackIcon;

	/**
	 * @brief 인게임 무기 스킬 UI 아이콘
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftObjectPtr<UTexture2D> WeaponSkillIcon;

	/**
	 * @brief 무기 분류 태그
	 * @details 예: Item.Type.Weapon.Melee.Sword, Item.Type.Weapon.Range.Staff
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Type", meta = (Categories = "Item.Type.Weapon"))
	FGameplayTag WeaponTag;

	// -----------------------------------------------------------------
	// [Animation] 공격 및 동작
	// -----------------------------------------------------------------

	/**
	 * @brief 무기 타입
	 * @details 무기 타입을 통해 캐릭터마다 무기에 맞는 몽타주를 재생합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Type")
	EWeaponType WeaponType;

	// -----------------------------------------------------------------
	// [GAS & Logic] 로직 연결
	// -----------------------------------------------------------------

	/**
	 * @brief 이 무기의 데미지 공식 GE
	 * @details 기본적으로 GE_DamageStandard를 할당합니다.
	 * 독단검, 화염검 등 특수 연산이 필요할 때만 다른 GE를 넣습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/**
	 * @brief 기본 공격 어빌리티 (Basic Attack / LMB)
	 * @details 무기 장착 시 부여되는 '평타' 로직입니다. (예: 검 휘두르기, 활 쏘기)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayAbility> BasicAttackAbility;

	/**
	 * @brief 무기 전용 스킬 (Weapon Skill / RMB or Q)
	 * @details 이 무기를 들었을 때만 쓸 수 있는 고유 기술입니다. (예: 올려치기, 멀티샷)
	 * 특정 키(Special Input)에 매핑됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayAbility> WeaponSkillAbility;

	/**
	 * @brief 투사체 클래스 (Projectile Class)
	 * @details 활이나 지팡이 등 원거리 무기가 발사할 액터 클래스입니다.
	 * @note 근거리 무기일 경우 비워둡니다 (None).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<AActor> ProjectileClass;

	// -----------------------------------------------------------------
	// [FX & Sound] 연출
	// -----------------------------------------------------------------

	/**
	 * @brief 공격 행동 전용, FX, Tags
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FX")
	FActionFXSettings ActionFX;
};

/**
 * @struct FArmorAssets
 * @brief 방어구 전용 에셋 데이터
 * @details 방어구는 주로 외형 변경이 핵심이며, 추가적인 패시브 효과(GE)를 가질 수 있습니다.
 */
USTRUCT(BlueprintType)
struct FArmorAssets : public FItemBaseAssets
{
	GENERATED_BODY()

public:
	/**
	* @brief 방어구 부위 태그
	* @details 예: Item.Type.Armor.Helmet, Item.Type.Armor.Chest
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Item.Type.Armor"))
	FGameplayTag ArmorTag;

	// -----------------------------------------------------------------
	// [GAS]
	// -----------------------------------------------------------------

	/**
	 * @brief 장착 효과 (Equip Gameplay Effect)
	 * @details 장착 시 캐릭터에게 적용할 패시브 버프(GE) 클래스입니다.
	 * 단순 스탯(방어력 등)은 Stats 테이블로 처리되므로, 특수 효과(예: 도트 힐, 오라)가 필요할 때만 사용합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<class UGameplayEffect> EquipGameplayEffect;
};

/**
 * @struct FSetBonusStats
 * @brief 세트 효과의 수치 및 규칙 데이터
 * @details 기획자가 밸런싱을 위해 수정하는 테이블입니다. (리소스 경로 없음)
 */
USTRUCT(BlueprintType)
struct FSetBonusStats : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** @brief 세트 이름 (예: "전설의 용기사 세트") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Info")
	FText SetName;

	// -------------------------------------------------------
	// [1단계: 수치형] (예: 2세트 공 +10%)
	// -------------------------------------------------------
	/** @brief 활성화에 필요한 세트 갯수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 1")
	int32 Slot1_Count = 2;

	/** @brief 적용할 스탯 (예: AttackPower) */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 1")
	//FGameplayAttribute Slot1_Attribute;
	/* @brief 적용할 스탯 태그
	 * @details "SetBonus.Stat" 하위 태그만 선택 가능하도록 필터링됨
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 1", meta = (Categories = "SetBonus.Stat"))
	FGameplayTag Slot1_AttributeTag;

	/** @brief 적용 수치 (GAS의 SetByCaller로 넘길 값) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 1")
	float Slot1_Value;

	/** @brief 세트 효과 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 1")
	FText Slot1_Desc;

	// -------------------------------------------------------
	// [2단계: 수치형] (예: 3세트 체력 +500)
	// -------------------------------------------------------
	
	/** @brief 활성화에 필요한 세트 갯수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 2")
	int32 Slot2_Count = 3;

	/** @brief 적용할 스탯 (예: AttackPower) */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 2")
	//FGameplayAttribute Slot2_Attribute;
	/* @brief 적용할 스탯 태그
	 * @details "SetBonus.Stat" 하위 태그만 선택 가능하도록 필터링됨
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 2", meta = (Categories = "SetBonus.Stat"))
	FGameplayTag Slot2_AttributeTag;

	/** @brief 적용 수치 (GAS의 SetByCaller로 넘길 값) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 2")
	float Slot2_Value;

	/** @brief 세트 효과 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 2")
	FText Slot2_Desc;

	// -------------------------------------------------------
	// [3단계: 기능형] (예: 4세트 번개 발사)
	// -------------------------------------------------------

	/** @brief 활성화에 필요한 세트 갯수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 3")
	int32 Slot3_Count = 4;

	/** @brief 세트 효과 발동을 위한 Value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 3")
	float Slot3_Value;

	/** @brief 세트 효과 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot 3")
	FText Slot3_Desc;

};

/**
 * @struct FSetBonusAssets
 * @brief 세트 효과의 리소스 및 로직 데이터
 * @details 프로그래머나 아티스트가 GE 클래스와 아이콘을 연결하는 테이블입니다.
 */
USTRUCT(BlueprintType)
struct FSetBonusAssets : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** @brief 세트 도감 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<UTexture2D> SetIcon;

	// -------------------------------------------------------
	// [1단계 리소스]
	// -------------------------------------------------------

	/*
	 * @brief 1단계 적용 GE 클래스
	 * @details Stats 테이블의 Slot1 조건 만족 시 적용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS Logic")
	TSoftClassPtr<UGameplayEffect> Slot1_Effect;

	// -------------------------------------------------------
	// [2단계 리소스]
	// -------------------------------------------------------

	/*
	 * @brief 2단계 적용 GE 클래스
	 * @details Stats 테이블의 Slot2 조건 만족 시 적용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS Logic")
	TSoftClassPtr<UGameplayEffect> Slot2_Effect;

	// -------------------------------------------------------
	// [3단계 리소스]
	// -------------------------------------------------------

	/*
	 * @brief 3단계 적용 GA 클래스
	 * @details Stats 테이블의 Slot3 조건 만족 시 적용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS Logic")
	TSoftClassPtr<UGameplayAbility> Slot3_Ability;
};