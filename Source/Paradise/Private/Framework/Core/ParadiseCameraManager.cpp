// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Core/ParadiseCameraManager.h"
#include "Framework/InGame/InGameController.h"
#include "Components/SquadControlComponent.h"
#include "Components/AutoCombatComponent.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Characters/Base/PlayerBase.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Components/MeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

AParadiseCameraManager::AParadiseCameraManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AParadiseCameraManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsUltimatePlaying) return;

    AInGameController* PC = Cast<AInGameController>(GetOwningPlayerController());
    if (!PC) return;

    UAutoCombatComponent* AutoComp = PC->GetAutoCombatComponent();
    USquadControlComponent* SquadComp = PC->GetSquadControlComponent();
    if (!AutoComp || !SquadComp) return;

    bool bIsAuto = AutoComp->IsAutoMode();
    bool bIsSquadWipedOut = SquadComp->bIsSquadWipedOut;

    // 💡 수동+생존 상태가 아닐 때만 스마트 카메라 로직 가동
    if (bIsAuto || bIsSquadWipedOut)
    {
        UpdateDynamicSmartCamera(DeltaTime, bIsAuto, bIsSquadWipedOut, PC->GetPawn());
    }
}

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

    UAutoCombatComponent* AutoComp = PC->GetAutoCombatComponent();
    USquadControlComponent* SquadComp = PC->GetSquadControlComponent();
    if (!AutoComp || !SquadComp) return;

    bool bIsAuto = AutoComp->IsAutoMode();
    bool bIsSquadWipedOut = SquadComp->bIsSquadWipedOut;

    // 💡 1. [수동 + 생존] 상태: 원래 쓰던 숄더뷰/탑뷰 등 캐릭터 기본 카메라 사용
    if (!bIsAuto && !bIsSquadWipedOut)
    {
        if (PC->GetPawn() && GetViewTarget() != PC->GetPawn())
        {
            PC->SetViewTargetWithBlend(PC->GetPawn(), CameraBlendTime, VTBlend_Cubic, 2.0f, true);
        }
    }
    // 💡 2. [그 외 모든 상태]: OverviewCameraActor를 드론 렌즈로 사용 (세부 이동은 Tick에서 처리)
    else
    {
        if (OverviewCameraActor && GetViewTarget() != OverviewCameraActor)
        {
            PC->SetViewTargetWithBlend(OverviewCameraActor, 1.5f, VTBlend_Cubic, 2.0f, true);
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

void AParadiseCameraManager::UpdateDynamicSmartCamera(float DeltaTime, bool bIsAuto, bool bIsWipedOut, APawn* ControlledPawn)
{
    if (!OverviewCameraActor)
    {
        InitializeOverviewCamera();
    }
    // 🚨 1번 체크: 관전 카메라(드론)가 존재하는가?
    if (!OverviewCameraActor)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Red, TEXT("❌ [SmartCamera] OverviewCameraActor가 없습니다!"));
        return;
    }

    FVector TargetCenter = FVector::ZeroVector;
    FString TargetName = TEXT("None"); // 디버그 출력용

    // ==========================================
    // 1. [누구를 비출 것인가?] 타겟 중심점 계산
    // ==========================================
    if (bIsWipedOut)
    {
        TArray<AActor*> Enemies;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), Enemies);

        int32 AliveEnemyCount = 0;
        for (AActor* Actor : Enemies)
        {
            if (AUnitBase* Enemy = Cast<AUnitBase>(Actor))
            {
                if (!Enemy->IsDead())
                {
                    TargetCenter += Enemy->GetActorLocation();
                    AliveEnemyCount++;
                }
            }
        }
        if (AliveEnemyCount > 0)
        {
            TargetCenter /= AliveEnemyCount;
            TargetName = FString::Printf(TEXT("적 무리 (%d명)"), AliveEnemyCount);
        }
        else
        {
            TargetCenter = LastDeathLocation;
            TargetName = TEXT("마지막 사망 위치(적 없음)");
        }
    }
    else if (ControlledPawn)
    {
        TargetCenter = ControlledPawn->GetActorLocation();
        TargetName = ControlledPawn->GetName();
    }
    else
    {
        TargetName = TEXT("타겟 소실(Pawn 없음)");
    }

    // ==========================================
    // 2. [얼마나 멀리서 비출 것인가?] 오프셋 계산
    // ==========================================
    FVector CameraOffset;
    if (bIsAuto)
    {
        CameraOffset = FVector(-1200.0f, 0.0f, 1500.0f);
    }
    else
    {
        CameraOffset = FVector(-800.0f, 0.0f, 1000.0f);
    }

    // ==========================================
    // 3. 카메라 부드럽게 이동 및 회전
    // ==========================================
    FVector TargetCamLoc = TargetCenter + CameraOffset;
    FRotator TargetCamRot = UKismetMathLibrary::FindLookAtRotation(TargetCamLoc, TargetCenter);

    FVector NewLoc = FMath::VInterpTo(OverviewCameraActor->GetActorLocation(), TargetCamLoc, DeltaTime, 2.5f);
    FRotator NewRot = FMath::RInterpTo(OverviewCameraActor->GetActorRotation(), TargetCamRot, DeltaTime, 2.5f);

    OverviewCameraActor->SetActorLocationAndRotation(NewLoc, NewRot);

    // 💡 [디버그 로그 출력] 키(Key) 값을 1번으로 주어 한 줄에서 계속 숫자가 갱신되도록 합니다.
    if (GEngine)
    {
        FString DebugMsg = FString::Printf(TEXT("📷 [SmartCamera 작동중]\n모드: %s | 전멸여부: %s\n현재 타겟: %s\n목표 좌표: %s\n현재 카메라 좌표: %s"),
            bIsAuto ? TEXT("자동(Auto)") : TEXT("수동(Manual)"),
            bIsWipedOut ? TEXT("전멸(O)") : TEXT("생존(X)"),
            *TargetName,
            *TargetCamLoc.ToString(),
            *NewLoc.ToString());

        GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Cyan, DebugMsg);
    }
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
