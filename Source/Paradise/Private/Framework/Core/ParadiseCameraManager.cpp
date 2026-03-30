// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Core/ParadiseCameraManager.h"
#include "Framework/InGame/InGameController.h"
#include "Components/SquadControlComponent.h"
#include "Components/AutoCombatComponent.h"
#include "Characters/Base/CharacterBase.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Characters/Base/PlayerBase.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Components/MeshComponent.h"
#include "Kismet/GameplayStatics.h"

void AParadiseCameraManager::InitializeOverviewCamera()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), OverviewCameraTag, FoundActors);
   // UE_LOG(LogTemp, Warning, TEXT("🔍 [Camera] 태그로 찾은 액터 수: %d개"), FoundActors.Num());
    if (FoundActors.Num() > 0)
    {
        OverviewCameraActor = FoundActors[0];
        /*UE_LOG(LogTemp, Log, TEXT("✅ [Camera] 태그 '%s'로 카메라 액터(%s)를 찾았습니다."),
            *OverviewCameraTag.ToString(), *OverviewCameraActor->GetName());*/
    }
}

void AParadiseCameraManager::UpdateCameraSystem()
{
    if (bIsUltimatePlaying) return;

    AInGameController* PC = Cast<AInGameController>(GetOwningPlayerController());
    if (!PC) return;

    // 우선순위 1: 전멸했거나 자동 모드일 때 -> Overview 카메라
    UAutoCombatComponent* AutoComp = PC->GetAutoCombatComponent();
    USquadControlComponent* SquadComp = PC->GetSquadControlComponent();

    if (!AutoComp || !SquadComp) return;

    bool bIsAuto = AutoComp ? AutoComp->IsAutoMode() : false;
    bool bIsSquadWipedOut = SquadComp->bIsSquadWipedOut;



    if ((bIsSquadWipedOut || bIsAuto) && OverviewCameraActor)
    {
        if (bIsSquadWipedOut)
        {
            //컨트롤러(나 자신)를 마지막 사망 위치로 이동시킴
            PC->ClientSetLocation(LastDeathLocation, LastDeathRotation);

            //뷰 타겟을 this 컨트롤러로 즉시 고정
            //전멸시 카메라 시점 이상한것 해결
            PC->SetViewTarget(this);
        }

        //Overview로 부드럽게 이동
        if (GetViewTarget() != OverviewCameraActor)
        {
            PC->SetViewTargetWithBlend(OverviewCameraActor, 1.5f, VTBlend_Cubic, 2.0f, true);
            UE_LOG(LogTemp, Log, TEXT("📷 [Camera] Overview 모드로 전환 (From Death Pos)"));
        }
    }
    // 우선순위 2: 조종 가능한 캐릭터가 있을 때 -> 캐릭터 카메라
    else if (PC->GetPawn())
    {
        if (GetViewTarget() != PC->GetPawn())
        {
            PC->SetViewTargetWithBlend(PC->GetPawn(), CameraBlendTime, VTBlend_Cubic, 2.0f, true);
            UE_LOG(LogTemp, Log, TEXT("📷 [Camera] 캐릭터 모드로 복귀"));
        }
    }
}


void AParadiseCameraManager::StartUltimateCamera(AActor* TargetActor)
{
    if (!TargetActor) return;

    if (bIsUltimatePlaying || CurrentUltimateTarget != nullptr)
    {
        StopUltimateCamera(nullptr); // 기존 겹침 방지 강제 종료
    }

    bIsUltimatePlaying = true;
    CurrentUltimateTarget = TargetActor;

    if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayerController()))
        InGamePC->SetActionPanelEnabled(false);

    //코드 정리
    SetUltimateActorsVisibility(TargetActor, true);     // 1. 주변 캐릭터 숨김 & 무적
    SetUltimateRenderingEffects(TargetActor, true);     // 2. 외곽선 및 핀 조명 렌더링 세팅
    SetUltimateTimeDilation(TargetActor, true);         // 3. 타임 딜레이 (슬로우 모션)
    SpawnUltimateLight(TargetActor);                    // 4. 전용 라이트 스폰

    //카메라 위치 계산 및 이동
    if (!UltimateCamera) UltimateCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass());

    FVector CamPos = TargetActor->GetActorLocation() + TargetActor->GetActorTransform().TransformVector(UltimateCameraOffset);
    FVector LookTarget = TargetActor->GetActorLocation() + TargetActor->GetActorTransform().TransformVector(UltimateLookAtOffset);

    UltimateCamera->SetActorLocationAndRotation(CamPos, (LookTarget - CamPos).Rotation());
    UltimateCamera->GetCameraComponent()->SetFieldOfView(UltimateCameraFOV);

    if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayerController()))
    {
        PC->SetViewTargetWithBlend(UltimateCamera, 0.15f, VTBlend_Cubic, 0.5f, true);
    }
}

void AParadiseCameraManager::StopUltimateCamera(AActor* RequestingActor)
{
    if (RequestingActor && RequestingActor != CurrentUltimateTarget) return;

    //원상 복구 로직 정리
    if (CurrentUltimateTarget)
    {
        SetUltimateTimeDilation(CurrentUltimateTarget, false);
        SetUltimateRenderingEffects(CurrentUltimateTarget, false);
    }
    SetUltimateActorsVisibility(nullptr, false);

    if (UltimateLightActor)
    {
        UltimateLightActor->Destroy();
        UltimateLightActor = nullptr;
    }

    bIsUltimatePlaying = false;
    CurrentUltimateTarget = nullptr;

    UpdateCameraSystem(); // 카메라 복귀

    bIsUltimatePlaying = true; // 복귀 중 궁극기 차단
    float DelayTime = CameraBlendTime > 0.0f ? CameraBlendTime : 1.0f;
    GetWorld()->GetTimerManager().SetTimer(UltimateCooldownTimerHandle, this, &AParadiseCameraManager::UnlockUltimateState, DelayTime, false);
}

void AParadiseCameraManager::BeginPlay()
{
    Super::BeginPlay();

    InitializeOverviewCamera();
}

void AParadiseCameraManager::UnlockUltimateState()
{
    bIsUltimatePlaying = false; // 이제 진짜로 궁극기 잠금 해제!
    if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayerController()))
    {
        InGamePC->SetActionPanelEnabled(true);
    }
    UE_LOG(LogTemp, Log, TEXT("✅ [Camera] 카메라 복귀 완료. 이제 다음 궁극기 사용이 가능합니다."));
}

void AParadiseCameraManager::SetUltimateTimeDilation(AActor* TargetActor, bool bEnable)
{
    if (bEnable)
    {
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), UltimateTimeDilation);
        if (UltimateTimeDilation > 0.0f)
            TargetActor->CustomTimeDilation = 1.0f / UltimateTimeDilation;
    }
    else
    {
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
        if (TargetActor) TargetActor->CustomTimeDilation = 1.0f;
    }
}

void AParadiseCameraManager::SetUltimateRenderingEffects(AActor* TargetActor, bool bEnable)
{
    if (!TargetActor) return;
    TArray<UMeshComponent*> Meshes;
    TargetActor->GetComponents<UMeshComponent>(Meshes);

    for (UMeshComponent* Mesh : Meshes)
    {
        Mesh->SetRenderCustomDepth(bEnable);
        Mesh->SetCustomDepthStencilValue(3);
        Mesh->SetLightingChannels(Mesh->LightingChannels.bChannel0, bEnable, Mesh->LightingChannels.bChannel2);
    }
}

void AParadiseCameraManager::SpawnUltimateLight(AActor* TargetActor)
{
    if (UltimateLightActor || !TargetActor) return;

    FVector LightPos = TargetActor->GetActorLocation() + TargetActor->GetActorTransform().TransformVector(UltimateLightOffset);
    UltimateLightActor = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), LightPos, FRotator::ZeroRotator);

    if (UPointLightComponent* LightComp = UltimateLightActor->PointLightComponent)
    {
        LightComp->SetIntensity(UltimateLightIntensity);
        LightComp->SetAttenuationRadius(UltimateLightRadius);
        LightComp->SetLightingChannels(false, true, false);
    }
}

void AParadiseCameraManager::SetUltimateActorsVisibility(AActor* TargetActor, bool bHide)
{
    if (bHide)
    {
        HiddenActors.Empty();
        TArray<AActor*> OutActors;
        TArray<AActor*> Ignore; Ignore.Add(TargetActor);
        TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes; ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

        UKismetSystemLibrary::SphereOverlapActors(GetWorld(), TargetActor->GetActorLocation(), 3000.0f, ObjTypes, ACharacterBase::StaticClass(), Ignore, OutActors);

        for (AActor* Actor : OutActors)
        {
            if (!Actor->IsHidden())
            {
                Actor->SetActorHiddenInGame(true);
                if (APlayerBase* Player = Cast<APlayerBase>(Actor))
                {
                    if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
                        ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Buff.Invincible"));
                    if (UAnimInstance* Anim = Player->GetMesh()->GetAnimInstance())
                        Anim->Montage_Stop(0.0f);
                }
                HiddenActors.Add(Actor);
            }
        }
    }
    else
    {
        for (auto& Actor : HiddenActors)
        {
            if (Actor.IsValid() && Actor->IsHidden())
            {
                Actor->SetActorHiddenInGame(false);
                if (APlayerBase* Player = Cast<APlayerBase>(Actor.Get()))
                {
                    if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
                        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Buff.Invincible"));
                }
            }
        }
        HiddenActors.Empty();
    }
}
