#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FXStructs.generated.h"

/**
 * @struct FFXPayload
 * @brief 사운드, 파티클 등 한 번에 재생될 연출 데이터의 묶음
 */
USTRUCT(BlueprintType)
struct FFXPayload
{
    GENERATED_BODY()

public:
    // 시각 효과 (나이아가라)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
    TSoftObjectPtr<class UNiagaraSystem> VisualEffect;

    // 청각 효과 (사운드)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
    TSoftObjectPtr<class USoundBase> SoundEffect;

    // 카메라 쉐이크
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
    TSubclassOf<UCameraShakeBase> CameraShake;

    // 크기 조절
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
    FVector Scale = FVector(1.0f);

    // 위치 오프셋 (필요하다면)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
    FVector LocationOffset = FVector::ZeroVector;
};

/**
 * @struct FReactionFXSettings
 * @brief 피격 및 생존 반응 전용 FX, Tags
 */
USTRUCT(BlueprintType)
struct FReactionFXSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Asset")
	TSoftObjectPtr<class UFXDataAsset> ReactionFXData; // 피격음, 피 튀기는 이펙트 등

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "Effect.Hit"))
	FGameplayTag HitTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "Effect.Death"))
	FGameplayTag DeathTag;
};

/**
 * @struct FReactionFXSettings
 * @brief 공격 행동 전용, FX, Tags
 */
USTRUCT(BlueprintType)
struct FActionFXSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Asset")
	TSoftObjectPtr<class UFXDataAsset> ActionFXData; // 무기 휘두르는 소리, 검기 이펙트 등

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "Effect.Attack"))
	FGameplayTag BasicAttackTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "Effect.Skill"))
	FGameplayTag SkillTag;
};

/**
 * @struct FReactionFXSettings
 * @brief 궁극기 전용, FX, Tags
 */
USTRUCT(BlueprintType)
struct FUltimateFXSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Asset")
	TSoftObjectPtr<class UFXDataAsset> UltimateFXData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "Effect.Ultimate"))
	FGameplayTag UltimateTag;
};
