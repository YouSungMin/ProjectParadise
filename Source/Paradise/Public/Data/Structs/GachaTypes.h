#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/Enums/GameEnums.h"
#include "GachaTypes.generated.h"

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

	/** @brief 아이템 등급 (UI 연출 분기용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha")
	EItemRarity Rarity = EItemRarity::Common;

	/** @brief 등장 가중치 (픽업 이벤트 시 이 값을 높임) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	/** @brief 중복 획득 시 변환될 조각(파편)의 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Gacha", meta = (ClampMin = "0"))
	int32 DuplicateFragmentReward = 10;
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

	/** @brief 뽑힌 등급 (UI 박스 색상 및 이펙트 결정용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	EItemRarity PulledRarity = EItemRarity::Common;

	/** @brief 중복 여부 (true면 연출 시 "조각 변환!" 이펙트 출력) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	bool bIsDuplicate = false;

	/** @brief 중복 시 획득한 조각 개수 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	int32 ConvertedFragments = 0;

	/** @brief 이 뽑기가 진행되었을 때의 천장(Pity) 스택 (UI 표기용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Gacha")
	int32 CurrentPityCount = 0;
};
#pragma endregion 데이터 구조체