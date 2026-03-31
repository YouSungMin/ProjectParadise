// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Core/ParadiseCameraManager.h"
#include "Framework/InGame/InGameController.h"
#include "Components/SquadControlComponent.h"
#include "Components/AutoCombatComponent.h"
#include "Characters/Base/CharacterBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Objects/HomeBase.h"
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
        if (!OverviewCameraActor) return;
    }

    FVector TargetCenter = FVector::ZeroVector;
    FString TargetName = TEXT("None");

    //카메라 피로도(시간) 업데이트
    if (CurrentSmartTarget)
    {
        CurrentTargetFocusTime += DeltaTime;
    }

    // 2. 모든 캐릭터 순회하며 가중치 평가
    TArray<AActor*> AllCharacters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacterBase::StaticClass(), AllCharacters);

    AActor* BestTarget = nullptr;
    float HighestScore = -9999.0f;

    for (AActor* Actor : AllCharacters)
    {
        ACharacterBase* Character = Cast<ACharacterBase>(Actor);

        // 유효성, 생존, 기지 제외 체크
        if (!Character || Character->IsHidden() || Character->IsA<AHomeBase>()) continue;

        //사망한 캐릭터 체크
        if (Character->IsDead()) continue; 

        //기본 점수에 일정 범위의 무작위 점수를 부여
        float CurrentScore = FMath::RandRange(0.0f, 5.0f);

        //우선순위 설정
        if (Character == ControlledPawn)
        {
            CurrentScore += 40.0f; // 플레이어
            if (bIsWipedOut) CurrentScore -= 100.0f;
        }
        else if (Character->ActorHasTag(TEXT("Unit.Faction.Friendly.Familiar")))
        {
            CurrentScore += 30.0f; // 아군
        }
        else
        {
            CurrentScore += 20.0f; // 적군 
        }

        // 공격, 스킬, 궁극기를 사용중이면 점수 +
        if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
        {
            if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State")))
            {
                CurrentScore += 40.0f;
            }
        }

        //너무 오래 한대상을 비췄다면
        if (Character == CurrentSmartTarget)
        {
            if (CurrentTargetFocusTime > 6.0f) // 6초
            {
                CurrentScore -= 200.0f; //패널티 점수
            }
            else
            {
                CurrentScore += 50.0f; // 6초가 되기 전까지는 높은 보너스를 주어 타겟이 휙휙 바뀌는 멀미 방지
            }
        }

        // --- 최고 점수 갱신 ---
        if (CurrentScore > HighestScore)
        {
            HighestScore = CurrentScore;
            BestTarget = Character;
        }
    }

    //최종 타겟 확정 및 피로도 초기화
    if (BestTarget)
    {
        // 타겟이 새롭게 변경되었다면 시간 초기화
        if (CurrentSmartTarget != BestTarget)
        {
            CurrentSmartTarget = BestTarget;
            CurrentTargetFocusTime = 0.0f;
        }

        TargetCenter = BestTarget->GetActorLocation();
        TargetName = FString::Printf(TEXT("%s (Score: %.0f)"), *BestTarget->GetName(), HighestScore);
        LastDeathLocation = TargetCenter;
    }
    else
    {
        TargetCenter = LastDeathLocation;
        TargetName = TEXT("타겟 소실 (마지막 위치)");
    }

    FVector CameraOffset;
    float TargetFOV = 90.0f; // 기본 시야각

    if (bIsAuto || bIsWipedOut)
    {
        //자동 or 스쿼드 전멸시 카메라
        CameraOffset = FVector(0.0f, 2800.0f, 1000.0f);
        TargetFOV = 45.0f; // 일반적인 쿼터뷰/탑뷰 게임의 시야각
    }
    else
    {
        //수동모드 카메라
        CameraOffset = FVector(0.0f, 1315.0f, 480.0f);
        TargetFOV = 45.0f;
    }

    FVector TargetCamLoc = TargetCenter + CameraOffset;

    // Overview 카메라가 항상 타겟의 중앙을 바라보도록 회전값 계산
    FRotator TargetCamRot = UKismetMathLibrary::FindLookAtRotation(TargetCamLoc, TargetCenter);

    float InterpSpeed = 2.5f;

    // 위치와 회전 부드럽게 보간
    OverviewCameraActor->SetActorLocationAndRotation(
        FMath::VInterpTo(OverviewCameraActor->GetActorLocation(), TargetCamLoc, DeltaTime, InterpSpeed),
        FMath::RInterpTo(OverviewCameraActor->GetActorRotation(), TargetCamRot, DeltaTime, InterpSpeed)
    );

    //시야각 플레이어와 보간
    if (UCameraComponent* CamComp = OverviewCameraActor->FindComponentByClass<UCameraComponent>())
    {
        CamComp->SetFieldOfView(FMath::FInterpTo(CamComp->FieldOfView, TargetFOV, DeltaTime, InterpSpeed));
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
