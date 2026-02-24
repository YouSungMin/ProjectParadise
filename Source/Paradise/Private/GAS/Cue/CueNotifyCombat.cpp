// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Cue/CueNotifyCombat.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Data/Assets/FXDataAsset.h"

bool UCueNotifyCombat::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
    if (!FXDataAsset)
    {
        return false;
    }

    // 2. 전달받은 태그 (예: Effect.Hit.Sword)
    FGameplayTag RequestTag = Parameters.OriginalTag;

    // 3. 데이터 에셋에서 구조체 검색
    FFXPayload* FoundFX = FXDataAsset->FindEffect(RequestTag);

    // 4. 찾았다면 재생!
    if (FoundFX)
    {
        FVector SpawnLocation = Parameters.Location;

        if (Parameters.EffectContext.GetHitResult())
        {
            SpawnLocation = Parameters.EffectContext.GetHitResult()->ImpactPoint;
        }

        // (A) 나이아가라 재생 (동기 로드)
        if (!FoundFX->VisualEffect.IsNull())
        {
            UNiagaraSystem* VFX = FoundFX->VisualEffect.LoadSynchronous();
            if (VFX)
            {
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    GetWorld(),
                    VFX,
                    SpawnLocation + FoundFX->LocationOffset,
                    FRotator::ZeroRotator,
                    FoundFX->Scale
                );
            }
        }

        // (B) 사운드 재생 (동기 로드)
        if (!FoundFX->SoundEffect.IsNull())
        {
            USoundBase* SFX = FoundFX->SoundEffect.LoadSynchronous();
            if (SFX)
            {
                UGameplayStatics::PlaySoundAtLocation(GetWorld(), SFX, SpawnLocation);
            }
        }

        return true;
    }

    return false;
}
