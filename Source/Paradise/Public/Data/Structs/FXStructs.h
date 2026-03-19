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
 * @struct FWeaponFXSettings
 * @brief 무기 전용 FX 구조체 (Weapon)
 */
USTRUCT(BlueprintType)
struct FWeaponFXSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Asset")
	TSoftObjectPtr<class UFXDataAsset> FXData; // 검기, 휘두르는 소리 등

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag BasicAttackTag; // 평타 태그

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag SkillTag;       // 스킬 태그
};

/**
 * @struct FCharacterFXSettings
 * @brief 플레이어 캐릭터 전용 FX 구조체 (Character)
 */
USTRUCT(BlueprintType)
struct FCharacterFXSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Asset")
	TSoftObjectPtr<class UFXDataAsset> FXData; // 기합 소리, 피격음 등

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag BasicAttackTag; // 평타 시 기합

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag SkillTag;       // 스킬 시 기합

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag UltimateTag;    // 궁극기 연출

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag HitTag;         // 피격 태그

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag DeathTag;       // 사망 태그
};

/**
 * @struct FAIUnitFXSettings
 * @brief 몬스터 / 패밀리어 공통 FX 구조체
 */
USTRUCT(BlueprintType)
struct FAIUnitFXSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Asset")
	TSoftObjectPtr<class UFXDataAsset> FXData; // 몬스터 소리, 피격 등

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag BasicAttackTag; // 평타

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag HitTag;         // 피격

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Tags", meta = (Categories = "FX"))
	FGameplayTag DeathTag;       // 사망
};
