#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/Enums/GameEnums.h"
#include "GachaTypes.generated.h"

#pragma region 배너 및 확률 데이터
/**
 * @struct FGachaBannerData
 * @brief 기획자가 각 배너(픽업, 상시, 장비 등)의 전반적인 규칙과 확률을 세팅하는 마스터 테이블
 * @details 이 테이블 하나만 교체하면 가챠의 가격, 확률, 풀(Pool)이 모두 데이터 주도적으로 변경됩니다.
 */
USTRUCT(BlueprintType)
struct FGachaBannerData : public FTableRowBase
{
	GENERATED_BODY()

	/** @brief 배너의 종류 (캐릭터 or 장비) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Banner")
	EGachaBannerType BannerType = EGachaBannerType::Character;

	/** @brief 1회 소환에 필요한 에테르의 양 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Banner", meta = (ClampMin = "0"))
	int32 RequiredAether = 100;

	/** * @brief 등급별 등장 확률 (Rarity Rates)
	 * @details 총합이 반드시 1.0(100%)이 되도록 기획자가 엑셀에서 세팅해야 합니다.
	 * 예: Common(0.8), Rare(0.15), Legendary(0.05)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Banner")
	TMap<EItemRarity, float> RarityProbabilities;

	/** * @brief 이 배너에서 등장할 아이템 목록이 담긴 풀(Pool) 데이터 테이블
	 * @details FGachaPoolRow 구조체를 사용하는 데이터 테이블을 연결합니다.
	 * TSoftObjectPtr를 사용하여 메모리 낭비와 순환 참조를 방지합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Banner")
	TSoftObjectPtr<UDataTable> TargetPoolTable = nullptr;

	/** @brief 이 배너 전용 천장 스택 (예: 50회 뽑기 시 최고 등급 확정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Banner", meta = (ClampMin = "1"))
	int32 PityThreshold = 50;
};
#pragma endregion 배너 및 확률 데이터

#pragma region 데이터 구조체
/**
 * @struct FGachaPoolRow
 * @brief 기획자가 엑셀(CSV)로 제어할 가챠 풀 데이터
 */
USTRUCT(BlueprintType)
struct FGachaPoolRow : public FTableRowBase
{
	GENERATED_BODY()

	/** @brief 획득할 대상의 고유 ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha")
	FName ItemID = NAME_None;

	/** @brief UI 결과창에 표시할 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha")
	FName ItemDisplayName = NAME_None;                          

	//** @brief UI 결과창에 표시할 아이콘/일러스트 텍스처 (10연차 슬롯용 - 작은 이미지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha")
	TSoftObjectPtr<UTexture2D> ItemIcon = nullptr;

	/** @brief 1연차 전용 세로 카드 일러스트 텍스처 (전신 일러스트 등 큰 이미지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha")
	TSoftObjectPtr<UTexture2D> ItemCardIllust = nullptr;

	/** @brief 아이템 등급 (UI 연출 분기용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha")
	EItemRarity Rarity = EItemRarity::Common;

	/** @brief 등장 가중치 (픽업 이벤트 시 이 값을 높임) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	// ── 캐릭터 전용 ─────────────────────────────────────────────────────────

	/**
	 * @brief [캐릭터 전용] 리빌 시 보여줄 캐릭터 스켈레탈 메시
	 * @details 장비 배너는 비워둡니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha|Character")
	TSoftObjectPtr<USkeletalMesh> CharacterSkeletalMesh = nullptr;

	/** @brief [캐릭터 전용] 리빌 후 재생할 Idle 애니메이션 시퀀스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha|Character")
	TSoftObjectPtr<UAnimSequence> CharacterIdleAnim = nullptr;

	/**
	 * @brief 리빌 시 표시될 메시의 스케일 (캐릭터/장비 공통)
	 * @details 캐릭터마다 메시 크기가 다를 경우 기획자가 여기서 조절합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha")
	FVector RevealMeshScale = FVector(1.0f, 1.0f, 1.0f);

	// ── 장비 전용 ────────────────────────────────────────────────────────────

	/**
	 * @brief [장비 전용] 리빌 시 보여줄 장비 스태틱 메시
	 * @details 캐릭터 배너는 비워둡니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha|Equipment")
	TSoftObjectPtr<UStaticMesh> EquipmentStaticMesh = nullptr;
};

/**
 * @struct FGachaResult
 * @brief UI와 인벤토리에 전달될 단일 뽑기 결과 (SRP 완벽 분리)
 * @details UI는 이 데이터를 보고 실루엣/스킵 연출을 진행하며, 인벤토리는 보상을 추가합니다.
 */
USTRUCT(BlueprintType)
struct FGachaResult
{
	GENERATED_BODY()

	/** @brief 뽑힌 아이템 ID */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	FName PulledItemID = NAME_None;

	/** @brief 캐릭터인지 장비인지 구별하는 분기 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	bool bIsCharacter = true;

	/** @brief 10연차 슬롯용 아이콘 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	TObjectPtr<UTexture2D> ItemIcon = nullptr;

	/** @brief 1연차 카드용 전신 일러스트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	TObjectPtr<UTexture2D> ItemCardIllust = nullptr;

	/** @brief UI 결과창에 표시할 아이템 이름 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	FName ItemName = NAME_None;

	/** @brief 뽑힌 등급 (UI 박스 색상 및 이펙트 결정용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	EItemRarity PulledRarity = EItemRarity::Common;

	/** @brief 중복 여부 (true면 연출 시 "조각 변환!" 이펙트 출력) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	bool bIsDuplicate = false;

	/** @brief 이 뽑기가 진행되었을 때의 천장(Pity) 스택 (UI 표기용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	int32 CurrentPityCount = 0;

	// ── 캐릭터 전용 ─────────────────────────────────────────────────────────

	/** @brief [캐릭터 전용] 리빌 시 보여줄 스켈레탈 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha|Character")
	TObjectPtr<USkeletalMesh> CharacterSkeletalMesh = nullptr;

	/** @brief [캐릭터 전용] 리빌 후 재생할 Idle 애님시퀀스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha|Character")
	TObjectPtr<UAnimSequence> CharacterIdleAnim = nullptr;

	/** @brief 매시 스케일 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	FVector RevealMeshScale = FVector(1.0f, 1.0f, 1.0f);

	// ── 장비 전용 ────────────────────────────────────────────────────────────

	/** @brief [장비 전용] 리빌 시 보여줄 스태틱 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha|Equipment")
	TObjectPtr<UStaticMesh> EquipmentStaticMesh = nullptr;
};
#pragma endregion 데이터 구조체