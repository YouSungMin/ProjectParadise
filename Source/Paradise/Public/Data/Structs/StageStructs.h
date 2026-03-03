#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "StageStructs.generated.h"

/**
 * @struct FStageStats
 * @brief 스테이지의 '규칙'과 '보상' 데이터 (기획 밸런싱용)
 * @details 리소스 경로 없이 순수 데이터만 포함합니다.
 */
USTRUCT(BlueprintType)
struct FStageStats : public FTableRowBase
{
	GENERATED_BODY()

public:
	// =========================================================
	//  기본 규칙 (Rules)
	// =========================================================

	/**
	 * @brief 다음 스테이지 ID (Next Stage) - [추가됨]
	 * @details 클리어 시 해금되거나, '다음 스테이지' 버튼 클릭 시 이동할 ID입니다.
	 * 비워두면(None) 마지막 스테이지로 간주합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	FName NextStageID;

	/** @brief 제한 시간 (초 단위) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule", meta = (ClampMin = "0.0"))
	float TimeLimit;

	// =========================================================
	//  경제/보상 (Economy & Reward)
	// =========================================================

	/** @brief 클리어 골드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward", meta = (ClampMin = "0"))
	int32 ClearGold;

	/** @brief 클리어 에테르*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward", meta = (ClampMin = "0"))
	int32 ClearAether;

	/** @brief 클리어 경험치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward", meta = (ClampMin = "0"))
	int32 ClearExp;

	// =========================================================
	//  텍스트 정보 (Text)
	// =========================================================

	/** @brief 스테이지 이름 (예: "어둠의 숲") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText StageName;

	/** @brief 스테이지 설명 (예: "강력한 고블린이 출몰합니다.") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText Description;
};

/**
 * @struct FStageAssets
 * @brief 스테이지의 '시청각 리소스' 데이터 (아트/연출용)
 * @details UI 표시용 이미지나 BGM 등을 관리합니다.
 */
USTRUCT(BlueprintType)
struct FStageAssets : public FTableRowBase
{
	GENERATED_BODY()

public:

	// =========================================================
	//  핵심 리소스 (Core) 
	// =========================================================

	/**
	* @brief 이동할 레벨(맵) 에셋
	* @details TSoftObjectPtr를 사용하므로 에디터에서 드롭다운으로 안전하게 선택 가능합니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
	TSoftObjectPtr<UWorld> MapAsset;

	// =========================================================
	//  시각 정보 (Visual)
	// =========================================================

	/** @brief 스테이지 선택창 썸네일 이미지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	/** @brief 로딩 스크린 배경 이미지 (선택 사항) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<UTexture2D> LoadingImage;

	// =========================================================
	//  청각 정보 (Audio)
	// =========================================================

	/** @brief 해당 스테이지 진입 시 재생할 배경음악 (BGM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TSoftObjectPtr<USoundBase> BackgroundMusic;

	/** @brief 환경음 (바람 소리 등, 선택 사항) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TSoftObjectPtr<USoundBase> AmbienceSound;
};

/**
 * @struct FStageWaveDetail
 * @brief 스테이지별 웨이브 및 몬스터 스폰 정보
 * @details DT_StageWaveDetail 테이블의 구조체입니다.
 * 하나의 스테이지(StageID)가 여러 개의 웨이브 행(Row)을 가집니다.
 */
USTRUCT(BlueprintType)
struct FStageWaveDetail : public FTableRowBase
{
	GENERATED_BODY()

public:
	// =========================================================
	//  연결 정보 (Link)
	// =========================================================

	/**
	 * @brief 대상 스테이지 ID (Foreign Key)
	 * @details DT_StageInfo의 RowName과 일치해야 합니다. (예: "Stage_1_1")
	 * 어떤 스테이지에 소속된 웨이브인지 연결합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
	FName TargetStageID;

	/**
	 * @brief 웨이브 순서 (Wave Order)
	 * @details 몇 번째 웨이브인지 나타냅니다. (1, 2, 3...)
	 * 같은 StageID + 같은 WaveOrder를 가진 행들은 동시에 처리됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link", meta = (ClampMin = "1"))
	int32 WaveOrder;

	// =========================================================
	//  웨이브 설정 (Wave Setting)
	// =========================================================

	/**
	 * @brief 웨이브 시작 전 대기 시간 (Pre-Wave Delay)
	 * @details 이전 웨이브가 끝나고 이 웨이브가 시작되기 전 휴식 시간입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float PreWaveDelay;

	// =========================================================
	//  스폰 정보 (Spawn Info)
	// =========================================================

	/**
	 * @brief 소환할 몬스터 ID
	 * @details DT_EnemyStats의 RowName과 일치해야 합니다. (예: "Goblin_Warrior")
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FName MonsterID;

	/** @brief 소환할 마릿수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "1"))
	int32 SpawnCount;

	/**
	 * @brief 스폰 간격 (Spawn Interval)
	 * @details 마릿수가 여러 마리일 때, 몇 초 간격으로 나올지 설정합니다.
	 * 0이면 동시에 우르르 나옵니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0"))
	float SpawnInterval;
};