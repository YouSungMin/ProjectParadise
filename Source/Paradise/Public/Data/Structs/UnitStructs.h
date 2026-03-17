#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/Enums/GameEnums.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "Data/Structs/FXStructs.h"
#include "Data/Structs/CombatTypes.h"
#include "UnitStructs.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UTexture2D;
class UGameplayAbility;
class UAnimMontage;
class AAIController;
class UBehaviorTree;
class UBlackboardData;
class USoundBase;

/**
 * @struct FUnitBaseStats
 * @brief 모든 유닛(플레이어, 몬스터, 소환수)의 공통 기초 능력치를 정의하는 부모 구조체
 * @details GAS AttributeSet의 초기화 데이터로 사용됩니다.
 * 'Base' 접두사가 붙은 수치는 장비나 버프가 적용되지 않은 순수 스탯을 의미합니다.
 */
USTRUCT(BlueprintType)
struct FUnitBaseStats : public FTableRowBase
{
	GENERATED_BODY()
	/** @brief 소환 코스트 (재화) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 SummonCost;

	// =========================================================
	//  전투 스탯 (Combat Stats)
	// =========================================================

	/**
	 * @brief 최대 체력 (Max HP)
	 * @details 스폰 시 Health Attribute의 초기값으로 설정됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "1.0"))
	float BaseMaxHP;

	/**
	 * @brief 최대 마나 (Max MP)
	 * @details 스폰 시 Mana Attribute의 초기값으로 설정됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
	float BaseMaxMP;

	/**
	 * @brief 공격력 (Attack Power)
	 * @details 평타 및 스킬 데미지 계산의 기초가 되는 값입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
	float BaseAttackPower;

	/**
	 * @brief 방어력 (Defense)
	 * @details 데미지 감소율 계산에 사용됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
	float BaseDefense;

	/**
	 * @brief 평상시 이동 속도 (Move Speed)
	 * @details 언리얼 단위(cm/s)입니다. 보통 걷기는 300~400, 달리기는 600 정도입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (ClampMin = "0.0"))
	float BaseMoveSpeed;

	/**
	 * @brief 크리티컬 확률 (CritRate)
	 * @details 크리티컬 확률 계산에 사용됩니다. ( 단위: 0% =  0.0f, 100% = 1.0f)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseCritRate;
};

/**
 * @struct FCharacterStats
 * @brief 플레이어 캐릭터의 '성장 규칙'과 '고유 스킬' 데이터를 정의하는 구조체
 * @details FUnitBaseStats를 상속받아 기본 스탯을 포함하며, 레벨업 시 상승하는 수치(Per Level)와 궁극기 설정이 추가되었습니다.
 * 로비(내 정보)와 인게임(스탯 초기화)에서 공통으로 사용되는 핵심 데이터입니다.
 * 플레이어 캐릭터는 무기(Weapon)에 공격 속도와 사거리가 존재합니다.
 */
USTRUCT(BlueprintType)
struct FCharacterStats : public FUnitBaseStats
{
	GENERATED_BODY()

public:

	// =========================================================
	//  성장 스탯 (Combat Stats)
	// =========================================================

	/**
	 * @brief 레벨업 성장 테이블 ID (Level Up / Growth ID)
	 * @details 캐릭터가 다음 레벨로 가기 위한 '필요 경험치량'이나 '소모 재화'가 정의된 테이블의 RowName입니다.
	 * DT_CharacterLevelUp 테이블을 참조합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Growth Info")
	FName LevelUpCostId;

	/**
	 * @brief 레벨 업 당 최대 체력 증가량 (Growth Max HP)
	 * @details 레벨업 시 이 수치만큼 MaxHP가 영구적으로 증가합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Growth", meta = (ClampMin = "0.0"))
	float GrowthHPPerLevel;

	/**
	 * @brief 레벨 업 당 공격력 증가량 (Growth Attack)
	 * @details 레벨업 시 이 수치만큼 기본 공격력이 증가합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Growth", meta = (ClampMin = "0.0"))
	float GrowthAttackPerLevel;

	/**
	 * @brief 레벨업 당 방어력 증가량 (Growth Defense)
	 * @details 레벨업 시 이 수치만큼 기본 방어력이 증가합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Growth", meta = (ClampMin = "0.0"))
	float GrowthDefensePerLevel;

	// =========================================================
	//  스킬 스탯 (Skill Stats)
	// =========================================================

	/**
	 * @brief 스킬 - FActionStats 테이블의 ID로 사용
	 * @details 개별 액션의 수치를 정의 해둔 구조체의 RowName
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionLink")
	FName SkillActionID;
};

/**
 * @struct FAIUnitStats
 * @brief AI가 제어하는 유닛(몬스터, 패밀리어)이 추가로 가지는 전투 정보 구조체입니다.
 * @details 플레이어와 달리 무기(Weapon)에 의존하지 않고 자체적인 공격 주기와 사거리를 가집니다.
 */
USTRUCT(BlueprintType)
struct FAIUnitStats : public FUnitBaseStats
{
	GENERATED_BODY()

	// =========================================================
	// 기본 정보 (Basic Info)
	// =========================================================

	/**
	 * @brief 공격 타입 (Attack Type)
	 * @details AI의 사거리 판단 및 상성 로직에 사용됩니다.
	 * 예: Unit.Attack.Melee (근접), Unit.Attack.Range (원거리)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type", meta = (Categories = "Unit.Attack"))
	FGameplayTag AttackTypeTag;

	/**
	 * @brief 역할군 (Role)
	 * @details 어그로 수준 및 AI 우선순위 결정에 사용됩니다.
	 * 예: Unit.Role.Tanker, Unit.Role.Dealer, Unit.Role.Support
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type", meta = (Categories = "Unit.Role"))
	FGameplayTag RoleTypeTag;

	/**
	 * @brief 등급 (Rank)
	 * @details 보스 UI 표시 여부 및 CC기 면역 로직에 사용됩니다.
	 * 예: Unit.Rank.Normal, Unit.Rank.Elite, Unit.Rank.Boss
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type", meta = (Categories = "Unit.Rank"))
	FGameplayTag RankTypeTag;

	// =========================================================
	//   AI 전투 스탯 (AI Combat)
	// =========================================================

	/**
	 * @brief AI 기본 공격 액션 ID
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionLink")
	FName BasicAttackActionID;

	/**
	 * @brief AI 스킬 액션 ID 목록
	 * @details FAIUnitAssets의 SkillAbilities 배열과 인덱스가 1:1로 매칭되도록 구성합니다.
	 * 예: 인덱스 0 = 돌진 스킬, 인덱스 1 = 브레스 스킬
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionLink")
	TArray<FName> SkillActionIDs;
};

/**
 * @struct FEnemyStats
 * @brief 몬스터 전용 스탯 테이블 구조체
 */
USTRUCT(BlueprintType)
struct FEnemyStats : public FAIUnitStats
{
	GENERATED_BODY()

public:

};

/**
 * @struct FFamiliarStats
 * @brief 퍼밀리어 전용 스탯 테이블 구조체
 */
USTRUCT(BlueprintType)
struct FFamiliarStats : public FAIUnitStats
{
	GENERATED_BODY()

public:
	//0305 김성현 - 캐릭터 리스폰 기능 을 위해 베이스로 변수이동
	///** @brief 소환 코스트 (재화) */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	//int32 SummonCost;
};

/**
 * @struct FUnitBaseAssets
 * @brief 모든 유닛(플레이어, 몬스터, 퍼밀리어)의 공통 리소스 정의한 부모 구조체
 * @details 외형(Mesh), 기본 애니메이션(AnimBP), 생존 반응(Hit/Dead) 등 필수적인 에셋을 포함합니다.
 */
USTRUCT(BlueprintType)
struct FUnitBaseAssets : public FTableRowBase
{
	GENERATED_BODY()

public:
	// =========================================================
	//  기본 정보 (Basic Info) 
	// =========================================================

	/**
	 * @brief 소속 진영 (Faction)
	 * @details 피아식별(아군/적군)의 기준이 되는 핵심 태그입니다.
	 * 예: Unit.Faction.Friendly.Player, Unit.Faction.Enemy, Unit.Faction.Friendly.Familiar
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Info", meta = (Categories = "Unit.Faction"))
	FGameplayTag FactionTag;

	// =========================================================
	//  Visual & Anim (Common)
	// =========================================================

	/**
	 * @brief 유닛 외형 (Skeletal Mesh)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Common")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	/**
	 * @brief 애니메이션 블루프린트
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Common")
	TSubclassOf<UAnimInstance> AnimBlueprint;

	// =========================================================
	//  Reaction Animation
	// =========================================================

	/**
	 * @brief 피격(Hit) 리액션 몽타주
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Common")
	TSoftObjectPtr<UAnimMontage> HitMontage;

	/**
	 * @brief 사망(Dead) 연출 몽타주
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Common")
	TSoftObjectPtr<UAnimMontage> DeathMontage;

	// =========================================================
	//  Audio & FX (Physical Reaction)
	// =========================================================
	/**
	 * @brief 피격 및 생존 반응 전용 FX, Tag 구조체
	 * @details 맞았을 때 나는 피격음/피 효과, 사망 시 비명 소리
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|FX")
	FReactionFXSettings ReactionFX;
};

/**
 * @struct FWeaponAnimSet
 * @brief 특정 무기 타입을 들었을 때 재생할 애니메이션 세트
 */
USTRUCT(BlueprintType)
struct FWeaponAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSoftObjectPtr<UAnimMontage> BasicAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSoftObjectPtr<UAnimMontage> SkillMontage;
};

/**
 * @struct FCharacterAssets
 * @brief 플레이어 캐릭터 전용 리소스 (UI, 스킬 슬롯 등)
 */
USTRUCT(BlueprintType)
struct FCharacterAssets : public FUnitBaseAssets
{
	GENERATED_BODY()
public:
	/**
	*@brief 캐릭터 전용 : 태그를 자동으로 'Friendly.Player'로 설정함
	*/
	FCharacterAssets()
	{
		FactionTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Friendly.Player"));
	}

	// =========================================================
	//  UI (Player Only)
	// =========================================================

	/**
	 * @brief 인게임 HUD용 얼굴 아이콘.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftObjectPtr<UTexture2D> FaceIcon;

	/**
	 * @brief 캐릭터 선택/가챠 화면용 전신 일러스트.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftObjectPtr<UTexture2D> BodyIcon;

	/**
	 * @brief 캐릭터 전용 돌파 재화(조각) 아이콘
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftObjectPtr<UTexture2D> AwakeningPieceIcon;

	/**
	 * @brief 로비레벨에서 캐릭터 연출용 Idle 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UAnimSequence> IdleAnim;

	/**
	 * @brief 인게임 궁극기 UI 아이콘
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftObjectPtr<UTexture2D> UltimateIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	EItemRarity Rarity = EItemRarity::Common;
	// =========================================================
	//  GAS & Montage (Player Only)
	// =========================================================

	/** @brief 캐릭터 궁극기 세트 (어빌리티, 이펙트, 투사체 묶음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	FCombatAbilitySetup UltimateAttackSetup;

	/**
	 * @brief 궁극기 연출 몽타주 (Ultimate Montage)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Skill")
	TSoftObjectPtr<UAnimMontage> UltimateMontage;

	/** @brief 장착한 무기 종류에 따라 달라지는 몽타주 세트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Weapon")
	TMap<EWeaponType, FWeaponAnimSet> WeaponAnimMap;

	/** @brief 궁극기 사용 시 재생할 이펙트/사운드 키값 (예: Effect.Ultimate.Meteor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|FX", meta = (Categories = "FX"))
	FGameplayTag UltimateEffectTag;
};

/**
 * @struct FAIUnitAssets
 * @brief AI 제어 유닛(적, 퍼밀리어) 공통 리소스
 */
USTRUCT(BlueprintType)
struct FAIUnitAssets : public FUnitBaseAssets
{
	GENERATED_BODY()

public:

	/**
	 * @brief UI 표현을 위한 아이콘
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftObjectPtr<UTexture2D> FaceIcon;

	// =========================================================
	//  Visual (AI Specific)
	// =========================================================

	/**
	 * @brief 몬스터 크기 배율 (Scale)
	 * @details 기본값은 1.0입니다. 보스 몬스터 등 덩치를 키워야 할 때 1.5, 2.0 등으로 설정합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual", meta = (ClampMin = "0.1"))
	float Scale = 1.0f; // 초기화 필수

	/**
	 * @brief 기본 공격 몽타주
	 * @details 가장 기초적인 공격 모션입니다. (플레이어는 콤보의 시작, AI는 기본 평타)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Common")
	TSoftObjectPtr<UAnimMontage> AttackMontage;

	// =========================================================
	//  인공지능 (AI)
	// =========================================================

	/**
	 * @brief 스킬 연출 몽타주 목록
	 * @details SkillAbilities, SkillActionIDs 배열과 인덱스가 1:1로 매칭되도록 구성합니다.
	 * 예: 인덱스 0 = 돌진 몽타주, 인덱스 1 = 브레스 몽타주
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Skill")
	TArray<TSoftObjectPtr<UAnimMontage>> SkillMontages;

	/**
	 * @brief 사용할 AI 컨트롤러 클래스
	 * @details 몬스터의 두뇌 역할을 하는 컨트롤러 클래스(BP_EnemyController 등)입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TSubclassOf<AAIController> AIController;

	/**
	 * @brief 실행할 비헤이비어 트리 에셋
	 * @details 몬스터의 행동 패턴(이동, 공격, 스킬 사용 등)이 정의된 BT 에셋입니다
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TSoftObjectPtr<UBehaviorTree> BehaviorTree;

	/**
	 * @brief 사용할 블랙보드 데이터 에셋
	 * @details 비헤이비어 트리가 사용할 메모리(변수 저장소)입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TSoftObjectPtr<UBlackboardData> Blackboard;

	// =========================================================
	//  GAS 어빌리티 (Abilities)
	// =========================================================

	/** @brief 몬스터 기본 평타 세트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Common")
	FCombatAbilitySetup BasicAttackSetup;

	/** @brief 몬스터가 사용하는 다중 스킬 목록 (1번 스킬, 2번 스킬...) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Skill")
	TArray<FCombatAbilitySetup> SkillSetups;

	/**
	 * @brief 공격 행동 전용, FX, Tags
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|FX")
	FActionFXSettings ActionFX;


};

/**
 * @struct FEnemyAssets
 * @brief 몬스터 전용 에셋 테이블 구조체
 */
USTRUCT(BlueprintType)
struct FEnemyAssets : public FAIUnitAssets
{
	GENERATED_BODY()

public:
	/**
	 *@brief 적 전용 : 태그를 자동으로 'Friendly.Familiar'로 설정함
	 */
	FEnemyAssets()
	{
		FactionTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"));
	}

	/**
	 * @brief 스킬 연출 태그 목록
	 * @details 보스가 사용하는 스킬들의 연출 태그 리스트
	 * * 인덱스 0: 스킬1, 인덱스 1: 스킬2 ... 순서대로 매핑
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Skill", meta = (Categories = "Effect.Skill"))
	TArray<FGameplayTag> SkillEffectTags;
};

/**
 * @struct FFamiliarAssets
 * @brief 퍼밀리어 전용 에셋 테이블 구조체
 */
USTRUCT(BlueprintType)
struct FFamiliarAssets : public FAIUnitAssets
{
	GENERATED_BODY()

	/**
	 *@brief 패밀리어 전용 : 태그를 자동으로 'Friendly.Familiar'로 설정함
	 */
	FFamiliarAssets()
	{
		FactionTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Friendly.Familiar"));
	}
};
