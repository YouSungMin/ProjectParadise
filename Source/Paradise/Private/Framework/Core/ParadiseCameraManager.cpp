// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Core/ParadiseCameraManager.h"
#include "Framework/InGame/InGameController.h"
#include "Components/SquadControlComponent.h"
#include "Components/AutoCombatComponent.h"
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

void AParadiseCameraManager::BeginPlay()
{
    Super::BeginPlay();

    InitializeOverviewCamera();
}
