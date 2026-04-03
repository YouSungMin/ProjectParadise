#pragma once
#include "CoreMinimal.h"
#include "ResultUITypes.generated.h"

/**
 * @brief 스테이지 클리어 시 UI로 전달할 보상 데이터 모음
 */
USTRUCT(BlueprintType)
struct FStageClearRewardData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Reward")
	int32 EarnedStars = 0;		// 획득한 별 갯수 (1~3)

	UPROPERTY(BlueprintReadOnly, Category = "Reward")
	int32 AcquiredGold = 0;		// 획득 골드

	UPROPERTY(BlueprintReadOnly, Category = "Reward")
	int32 AcquiredExp = 0;		// 획득 경험치

	UPROPERTY(BlueprintReadOnly, Category = "Reward")
	int32 AcquiredAether = 0;	// 획득 에테르

	UPROPERTY(BlueprintReadOnly, Category = "Reward")
	FName AcquiredFamiliar;	// 획득 퍼밀리어
};