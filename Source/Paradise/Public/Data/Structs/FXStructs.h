#pragma once

#include "CoreMinimal.h"
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