#pragma once

#include "CoreMinimal.h"
#include "GameEnums.generated.h"

/**
 * @enum EGamePhase
 * @brief 게임 흐름 제어를 위한 상태 머신(FSM)의 주요 단계를 정의
 */
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	/** @brief [준비 상태] 전투 시작 전 카운트다운 단계  */
	Ready,	
	/** @brief [전투 상태] 메인 게임 플레이 및 스폰/타이머 활성화 */
	Combat,	
	/** @brief [승리 상태] 클리어 조건 달성 및 보상 처리 */
	Victory,
	/** @brief [패배 상태] 타임오버 또는 사망으로 인해 패배한 상태 */
	Defeat,	
	/** @brief [결과 상태] 최종 결과창을 표시하고 로비 이동을 대기하는 단계 */
	Result
};

/**
 * @enum ECurrencyType
 * @brief 게임 내에서 사용되는 재화(Currency)의 종류를 정의하는 열거형입니다.
 */
UENUM(BlueprintType)
enum class ECurrencyType : uint8
{
	/** * @brief 게임 플레이를 통해 주로 획득하는 기본 재화 (골드)
	 * @details 캐릭터 레벨업, 장비 강화 등 기본적인 성장 시스템 전반에 범용적으로 소모됩니다.
	 */
	Gold			UMETA(DisplayName = "Gold (강화/레벨업)"),

	/** * @brief보상으로 획득하는 고급/유료 재화 (다이아/보석)
	 * @details 프리미엄 뽑기(가챠), 특수 아이템 구매, 행동력 충전 등 핵심 과금 모델에 사용됩니다.
	 */
	Aether				UMETA(DisplayName = "Aether (뽑기/고급재화)"),
	/** * @brief 소환(가챠) 시스템 전용 재화 (뽑기권)
	 * @details 이벤트 보상이나 상점 구매를 통해 얻으며, 영웅이나 퍼밀리어를 뽑을 때 1회용으로 소모됩니다.
	 */
	SummonTicket	UMETA(DisplayName = "Summon Ticket (뽑기권)")
};


/**
 * @enum EUnitType
 * @brief 몬스터, 퍼밀리어의 전투 타입 (근거리, 원거리, 보스 등)
 */
UENUM(BlueprintType)
enum class EAIUnitType : uint8
{
	/** @brief 타입 없음 */
	None		UMETA(DisplayName = "None"),
	/** @brief 근거리 공격 타입 */
	Melee		UMETA(DisplayName = "Melee (근거리)"),
	/** @brief 원거리 공격 타입 */
	Ranged		UMETA(DisplayName = "Ranged (원거리)"),
	/** @brief 지원가 버프 타입 */
	Support		UMETA(DisplayName = "Support (지원가)"),
	/** @brief 자폭 공격 타입 */
	Siege		UMETA(DisplayName = "Siege (공성/자폭)"),
	/** @brief 보스 몬스터 타입 */
	Boss		UMETA(DisplayName = "Boss (보스)")
};

/**
 * @enum EItemRarity
 * @brief 아이템의 희귀도 등급 (Tier) 정의
 * @details 드랍 확률, UI 테두리 색상, 스탯 랜덤 보정치 계산 등에 사용되는 핵심 척도입니다.
 * 보통 1성(Common)부터 5성(Legendary)까지의 단계로 구분됩니다.
 */
	UENUM(BlueprintType)
	enum class EItemRarity : uint8
{
	/** @brief 가장 흔한 등급 (회색 테두리, 1성) */
	Common		UMETA(DisplayName = "Common (1 Star)"),

	/** @brief 일반적이지 않은 등급 (초록색 테두리, 2성) */
	Uncommon	UMETA(DisplayName = "Uncommon (2 Star)"),

	/** @brief 희귀 등급 (파란색 테두리, 3성) */
	Rare		UMETA(DisplayName = "Rare (3 Star)"),

	/** @brief 영웅 등급 (보라색 테두리, 4성) */
	Epic		UMETA(DisplayName = "Epic (4 Star)"),

	/** @brief 전설 등급 (금색 테두리, 5성) */
	Legendary	UMETA(DisplayName = "Legendary (5 Star)")
};

/**
 * @enum ECombatActionType
 * @brief 현재 수행하려는 공격의 종류를 구분합니다.
 * @details 어빌리티가 이 타입을 인자로 전달하면, 캐릭터는 그에 맞는 데이터(계수, 몽타주 등)를 반환합니다.
 */
UENUM(BlueprintType)
enum class ECombatActionType : uint8
{
	BasicAttack,  // 기본 공격 (평타, LMB) - 계수 1.0
	WeaponSkill,  // 무기 스킬 (RMB, Q 등) - 계수 1.5 (WeaponStats 참조)
	UltimateSkill // 궁극기 (R) - 계수 3.0 (CharacterStats 참조)
};

/**
 * @enum EEquipmentSlot
 * @brief 장비 컴포넌트의 장비 슬롯 ENUM
 */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	/** @brief 장착 슬롯 : 무기 */
	Weapon		UMETA(DisplayName = "장착 슬롯 : 무기"),
	/** @brief 장착 슬롯 : 모자  */
	Hat		UMETA(DisplayName = "장착 슬롯 : 모자"),
	/** @brief 장착 슬롯 : 갑옷  */
	Armor		UMETA(DisplayName = "장착 슬롯 : 갑옷"),
	/** @brief 장착 슬롯 : 목걸이 */
	Necklace		UMETA(DisplayName = "장착 슬롯 : 목걸이"),
	/** @brief 장착 슬롯 : 반지  */
	Ring		UMETA(DisplayName = "장착 슬롯 : 반지"),
	/** @brief 장착 슬롯 : 없음 */
	None		UMETA(DisplayName = "장착 슬롯 : 없음"),
	/** @brief 장착 슬롯 : 오류 */
	Unknown		UMETA(DisplayName = "장착 슬롯 : 오류")
};

/**
 * @enum EEquipmentSlot
 * @brief 장비 컴포넌트의 장비 슬롯 ENUM
 */
UENUM(BlueprintType)
enum class EInputID : uint8
{
	None,       // 0
	Confirm,    // 1 (확인)
	Cancel,     // 2 (취소)
	Attack,     // 3 (평타)
	Skill,      // 4 (무기스킬)
	Ultimate    // 5 (궁극기)
};

/**
 * @enum EWeaponType
 * @brief 장착한 무기에 따른 애니메이션 및 로직 분기용
 */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Base	UMETA(DisplayName = "Base (맨손/기본)"),
	Melee	UMETA(DisplayName = "Melee (근접)"),
	Gun		UMETA(DisplayName = "Gun (총)"),
	Book	UMETA(DisplayName = "Book (마법책/지팡이)")
};

/**
 * @enum EFXEventType
 * @brief 연출(FX)이 필요한 상황을 정의합니다. 큐 노티파이에서 이 값을 넘겨 페이로드를 요청합니다.
 */
UENUM(BlueprintType)
enum class EFXEventType : uint8
{
	Hit			UMETA(DisplayName = "Hit (피격)"),
	Death		UMETA(DisplayName = "Death (사망)"),
	BasicAttack	UMETA(DisplayName = "Basic Attack (기본 공격)"),
	Skill		UMETA(DisplayName = "Weapon Skill (스킬)"),
	Ultimate	UMETA(DisplayName = "Ultimate (궁극기)")
};

/**
 * @enum ESocketTargetType
 * @brief 소켓을 찾을 대상을 지정하는 Enum 선언
 */
UENUM(BlueprintType)
enum class ESocketTargetType : uint8
{
	CharacterBody UMETA(DisplayName = "캐릭터 본체 (Character Body)"),
	EquippedWeapon UMETA(DisplayName = "장착된 무기 (Equipped Weapon)")
};
