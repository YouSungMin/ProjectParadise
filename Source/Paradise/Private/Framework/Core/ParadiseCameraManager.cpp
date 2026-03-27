// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Core/ParadiseCameraManager.h"
#include "Framework/InGame/InGameController.h"
#include "Components/SquadControlComponent.h"
#include "Components/AutoCombatComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Components/MeshComponent.h"
#include "Kismet/GameplayStatics.h"

void AParadiseCameraManager::InitializeOverviewCamera()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), OverviewCameraTag, FoundActors);
    UE_LOG(LogTemp, Warning, TEXT("🔍 [Camera] 태그로 찾은 액터 수: %d개"), FoundActors.Num());
    if (FoundActors.Num() > 0)
    {
        OverviewCameraActor = FoundActors[0];
        UE_LOG(LogTemp, Log, TEXT("✅ [Camera] 태그 '%s'로 카메라 액터(%s)를 찾았습니다."),
            *OverviewCameraTag.ToString(), *OverviewCameraActor->GetName());
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
    AInGameController* PC = Cast<AInGameController>(GetOwningPlayerController());
    if (!PC) return;

    bIsUltimatePlaying = true;
    CurrentUltimateTarget = TargetActor; // 복구를 위해 현재 스킬 시전자를 기억해둠

    // 1. 카메라 스폰
    if (!UltimateCamera)
    {
        UltimateCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass());
    }

    // 설정한 오프셋(Offset) 값을 기반으로 위치 계산
    FVector TargetLoc = TargetActor->GetActorLocation();
    FVector ForwardDir = TargetActor->GetActorForwardVector();
    FVector RightDir = TargetActor->GetActorRightVector();
    FVector UpDir = TargetActor->GetActorUpVector();

    // 캐릭터의 앞/뒤, 좌/우, 위/아래 방향에 맞춰서 오프셋을 적용
    FVector CamPos = TargetLoc + (ForwardDir * UltimateCameraOffset.X) + (RightDir * UltimateCameraOffset.Y) + (UpDir * UltimateCameraOffset.Z);
    FVector LookAtTarget = TargetLoc + (ForwardDir * UltimateLookAtOffset.X) + (RightDir * UltimateLookAtOffset.Y) + (UpDir * UltimateLookAtOffset.Z);

    FRotator CamRot = (LookAtTarget - CamPos).Rotation();

    UltimateCamera->SetActorLocationAndRotation(CamPos, CamRot);
    UltimateCamera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount = true;
    UltimateCamera->GetCameraComponent()->PostProcessSettings.MotionBlurAmount = 0.0f;
    UltimateCamera->GetCameraComponent()->SetFieldOfView(UltimateCameraFOV);

    // 카메라 트랜지션
    PC->SetViewTargetWithBlend(UltimateCamera, 0.15f, VTBlend_Cubic, 0.5f, true);

    // 슬로우 모션 및 시전자 속도 상쇄
    // 월드 전체를 설정한 배율(예: 0.3)로 느리게 만듭니다.
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), UltimateTimeDilation);

    // 캐릭터만 정상 속도로 움직이도록 설정
    if (UltimateTimeDilation > 0.0f)
    {
        TargetActor->CustomTimeDilation = 1.0f / UltimateTimeDilation;
    }

    // 🌟 1. CustomDepth(어둠 면역) + LightingChannel(전용 빛 수신) 동시 적용
    if (TargetActor)
    {
        TArray<UMeshComponent*> Meshes;
        TargetActor->GetComponents<UMeshComponent>(Meshes);
        for (UMeshComponent* Mesh : Meshes)
        {
            //커스텀 뎁스와 스텐실 켜기
            Mesh->SetRenderCustomDepth(true);
            Mesh->SetCustomDepthStencilValue(3);

            //전용 핀 조명(Channel 1)을 받을 수 있도록 채널을 켭니다.
            bool bCh0 = Mesh->LightingChannels.bChannel0;
            bool bCh2 = Mesh->LightingChannels.bChannel2;
            Mesh->SetLightingChannels(bCh0, true, bCh2);
        }
    }

    //오직 시전자만 비추는 전용 핀 조명 스폰
    if (!UltimateLightActor)
    {
        FVector LightPos = TargetLoc + (ForwardDir * UltimateLightOffset.X) + (RightDir * UltimateLightOffset.Y) + (UpDir * UltimateLightOffset.Z);

        UltimateLightActor = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), LightPos, FRotator::ZeroRotator);

        if (UPointLightComponent* LightComp = UltimateLightActor->PointLightComponent)
        {
           //빛 강도와 반경 설정
            LightComp->SetIntensity(UltimateLightIntensity);
            LightComp->SetAttenuationRadius(UltimateLightRadius);

            //'Channel 1'이 켜진 캐릭터만 비춥니다
            LightComp->SetLightingChannels(false, true, false);
        }
    }
}

void AParadiseCameraManager::StopUltimateCamera()
{
    bIsUltimatePlaying = false;

    // 월드의 시간을 원래대로(1.0) 복구합니다.
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

    if (CurrentUltimateTarget)
    {
        TArray<UMeshComponent*> Meshes;
        CurrentUltimateTarget->GetComponents<UMeshComponent>(Meshes);
        for (UMeshComponent* Mesh : Meshes)
        {
            //연출이 끝났으므로 마스크와 빛 채널을 모두 끄기
            Mesh->SetRenderCustomDepth(false);

            bool bCh0 = Mesh->LightingChannels.bChannel0;
            bool bCh2 = Mesh->LightingChannels.bChannel2;
            Mesh->SetLightingChannels(bCh0, false, bCh2);
        }

        CurrentUltimateTarget->CustomTimeDilation = 1.0f;
        CurrentUltimateTarget = nullptr;
    }

    // 켜두었던 핀 조명 파괴
    if (UltimateLightActor)
    {
        UltimateLightActor->Destroy();
        UltimateLightActor = nullptr;
    }

    UpdateCameraSystem();
}

void AParadiseCameraManager::BeginPlay()
{
    Super::BeginPlay();

    InitializeOverviewCamera();
}
