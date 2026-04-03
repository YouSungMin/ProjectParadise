// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SquadControlComponent.h"
#include "Framework/InGame/InGameController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/Core/ParadiseCameraManager.h"
#include "NavigationSystem.h"
#include "GameFramework/PlayerStart.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/Player/PlayerData.h"
#include "AI/Squad/SquadAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Paradise/Paradise.h"

#include "UI/HUD/Ingame/InGameHUDWidget.h"
#include "UI/Panel/Ingame/ActionControlPanel.h"

USquadControlComponent::USquadControlComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

AInGameController* USquadControlComponent::GetOwnerPC() const
{
    return Cast<AInGameController>(GetOwner());
}

const TArray<TObjectPtr<APlayerBase>>& USquadControlComponent::GetActiveSquadPawns() const
{
    return ActiveSquadPawns;
}



int32 USquadControlComponent::GetCurrentControlledIndex() const
{
    return CurrentControlledIndex;
}

AParadiseCameraManager* USquadControlComponent::GetParadiseCameraManager()
{
    //찾아둔게 있다면 그것을 리턴
    if (CachedCameraManager.IsValid())
    {
        return CachedCameraManager.Get();
    }

    //못찾았으면 캐스팅 후 약참조 저장
    if (AInGameController* PC = GetOwnerPC())
    {
        CachedCameraManager = Cast<AParadiseCameraManager>(PC->PlayerCameraManager);
    }

    return CachedCameraManager.Get();
}

void USquadControlComponent::RequestSwitchPlayer(int32 PlayerIndex)
{
    AInGameController* PC = GetOwnerPC();
    if (!PC) return;

    if (!ActiveSquadPawns.IsValidIndex(PlayerIndex) || ActiveSquadPawns[PlayerIndex] == nullptr)
    {
        //UE_LOG(LogParadiseSquad, Warning, TEXT("⚠️ [Controller] 잘못된 인덱스 요청이거나 빈 슬롯입니다: %d"), PlayerIndex);
        return;
    }

    APlayerBase* NewPlayer = ActiveSquadPawns[PlayerIndex];
    APlayerBase* OldPlayer = Cast<APlayerBase>(PC->GetPawn());

    if (!NewPlayer || NewPlayer == OldPlayer) return;
    if (NewPlayer && NewPlayer->IsDead()) return;

    if (AController* NewPawnController = NewPlayer->GetController())
    {
        //컴포넌트(this)가 아닌 컨트롤러(PC)와 비교
        if (NewPawnController != PC)
        {
            NewPawnController->UnPossess();
            NewPawnController->Destroy();
        }
    }

    //빙의 권한은 컨트롤러에게 있음
    PC->Possess(NewPlayer);
    CurrentControlledIndex = PlayerIndex;
    PC->SetViewTarget(NewPlayer);

    if (OldPlayer && !OldPlayer->IsDead())
    {
        PossessAI(OldPlayer);
    }

    for (APlayerBase* Member : ActiveSquadPawns)
    {
        if (Member && Member != NewPlayer)
        {
            if (ASquadAIController* SquadAI = Cast<ASquadAIController>(Member->GetController()))
            {
                SquadAI->SetLeader(NewPlayer);
            }
        }
    }

    /*FString Msg = FString::Printf(TEXT("Switch -> Hero %d"), PlayerIndex + 1);
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, Msg);*/

    /*UE_LOG(LogParadiseSquad, Warning, TEXT("🔄 [Controller] 캐릭터 교체 완료 (%s -> %s)"),
        OldPlayer ? *OldPlayer->GetName() : TEXT("None"),
        *NewPlayer->GetName());*/

    //UI/카메라 업데이트 명령 하달
    if (AParadiseCameraManager* CamManager = GetParadiseCameraManager())
    {
        CamManager->UpdateCameraSystem();
    }
    if (UInGameHUDWidget* HUD = PC->GetOrCreateInGameHUD())
    {
        if (UActionControlPanel* ActionPanel = HUD->GetActionControlPanel())
        {
            ActionPanel->RefreshActionPanel(PlayerIndex);
        }
    }

    if (OnPlayerSwitched.IsBound())
    {
        OnPlayerSwitched.Broadcast(PlayerIndex);
    }
}

void USquadControlComponent::RespawnSquadPlayer(int32 PlayerIndex)
{
    AInGameController* PC = GetOwnerPC();
    if (!PC) return;

    //PlayerState 가져오기
    AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>();
    if (!PS || !ActiveSquadPawns.IsValidIndex(PlayerIndex))
    {
        //UE_LOG(LogTemp, Error, TEXT("❌ [Respawn] 잘못된 인덱스거나 PS가 없습니다."));
        return;
    }

    APlayerData* Soul = PS->GetSquadMemberData(PlayerIndex);
    if (!Soul) return;

    if (ActiveSquadPawns[PlayerIndex] && !ActiveSquadPawns[PlayerIndex]->IsDead())
    {
        //UE_LOG(LogTemp, Warning, TEXT("⚠️ [Respawn] 해당 멤버는 이미 살아있습니다."));
        return;
    }

   // UE_LOG(LogTemp, Warning, TEXT("✨ [Respawn] 멤버 %d (%s) 부활 시퀀스 시작!"), PlayerIndex, *Soul->GetName());

    //위치 및 회전 계산을 헬퍼 함수
    FVector PlayerSpawnLocation;
    FRotator PlayerSpawnRotation;
    GetSafeRespawnLocationAndRotation(PC, PlayerSpawnLocation, PlayerSpawnRotation);

    UClass* SpawnClass = APlayerBase::StaticClass();
    if (PlayerBaseClass) {
        SpawnClass = PlayerBaseClass;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APlayerBase* NewBody = GetWorld()->SpawnActor<APlayerBase>(SpawnClass, PlayerSpawnLocation, PlayerSpawnRotation, SpawnParams);

    if (NewBody)
    {
        Soul->ResetStateForRespawn();
        NewBody->InitializePlayer(Soul);
        ActiveSquadPawns[PlayerIndex] = NewBody;
        Soul->bIsDead = false;

        //전멸 판정 시 PC->GetPawn 사용
        if (bIsSquadWipedOut || PC->GetPawn() == nullptr)
        {
            //UE_LOG(LogTemp, Warning, TEXT("✨ [Respawn] 전멸 위기에서 %s 부활! 제어권을 획득합니다."), *NewBody->GetName());

            bIsSquadWipedOut = false;
            RequestSwitchPlayer(PlayerIndex);

            FInputModeGameOnly GameInputMode;
            PC->SetInputMode(GameInputMode);
        }
        else
        {
            if (PlayerIndex != CurrentControlledIndex)
            {
                PossessAI(NewBody);
            }
        }
    }
}

void USquadControlComponent::GetSafeRespawnLocationAndRotation(AInGameController* PC, FVector& OutLocation, FRotator& OutRotation)
{
    FVector DesiredLocation = FVector::ZeroVector;
    OutLocation = FVector::ZeroVector;
    OutRotation = FRotator::ZeroRotator;

    if (!PC) return;

    //기준 위치(DesiredLocation) 설정
    if (APawn* LeaderPawn = PC->GetPawn())
    {
        // 1순위: 현재 리더의 뒤쪽 150 위치
        DesiredLocation = LeaderPawn->GetActorLocation() - (LeaderPawn->GetActorForwardVector() * 150.0f);
        OutRotation = LeaderPawn->GetActorRotation();
    }
    else
    {
        // 2순위 (전멸): PlayerStart 지점을 찾아 그 위치를 목표로 함
        if (AActor* PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
        {
            DesiredLocation = PlayerStart->GetActorLocation();
            OutRotation = PlayerStart->GetActorRotation();
        }
        else
        {
            DesiredLocation = FVector(0, 0, 200.0f);
        }
    }

    //네비메쉬 기반 안전 위치 보정 (NavMesh Projection)
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSystem)
    {
        FNavLocation ProjectedNavLoc;
        // 목표 위치 반경 (수평 200, 수직 500) 이내에서 가장 가까운 네비메쉬 바닥을 찾음
        if (NavSystem->ProjectPointToNavigation(DesiredLocation, ProjectedNavLoc, FVector(200.0f, 200.0f, 500.0f)))
        {
            OutLocation = ProjectedNavLoc.Location + FVector(0.0f, 0.0f, 50.0f);
        }
        else
        {
            // 목표 위치에 네비메쉬가 없다면 (벽 속 등) -> 리더 주변 랜덤 네비메쉬 포인트를 찾음
            if (APawn* LeaderPawn = PC->GetPawn())
            {
                if (NavSystem->GetRandomPointInNavigableRadius(LeaderPawn->GetActorLocation(), 300.0f, ProjectedNavLoc))
                {
                    OutLocation = ProjectedNavLoc.Location + FVector(0.0f, 0.0f, 50.0f);
                    //UE_LOG(LogTemp, Warning, TEXT("⚠️ [Respawn] 뒤쪽이 벽입니다. 리더 근처 안전 지대로 우회 스폰합니다."));
                }
            }
            else
            {
                OutLocation = DesiredLocation + FVector(0, 0, 50.0f);
            }
        }
    }
    else
    {
        // 맵에 NavMesh 자체가 없을 경우 예외 처리
        OutLocation = DesiredLocation + FVector(0, 0, 50.0f);
    }
}

void USquadControlComponent::InitializeSquadPawns()
{
    AInGameController* PC = GetOwnerPC();
    if (!PC) return;

    //PS 가져오기
    AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>();
    if (!PS) return;

    //UE_LOG(LogParadiseSquad, Warning, TEXT("🎮 [Controller] 육체(Pawn) 소환 시작..."));

    int32 SquadSize = PS->GetSquadSize();
    ActiveSquadPawns.Init(nullptr, SquadSize);
    int32 FirstValidIndex = -1;

    FVector BaseSpawnLoc = FVector(0.0f, 0.0f, 100.0f); // 실패 시 기본 좌표
    FRotator BaseSpawnRot = FRotator::ZeroRotator;

    //플레이어 스타트 위치 찾기
    if (AActor* PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
    {
        BaseSpawnLoc = PlayerStart->GetActorLocation();
        BaseSpawnRot = PlayerStart->GetActorRotation();
        //UE_LOG(LogParadiseSquad, Log, TEXT("✅ [Controller] 맵에서 PlayerStart 지점을 찾았습니다. 위치: %s"), *BaseSpawnLoc.ToString());
    }
    else
    {
        //UE_LOG(LogParadiseSquad, Warning, TEXT("⚠️ [Controller] 맵에 PlayerStart가 없습니다! 기본 위치(0,0,100)에서 스폰합니다."));
    }

    //스쿼드 스폰
    for (int32 i = 0; i < SquadSize; i++)
    {
        APlayerData* Soul = PS->GetSquadMemberData(i);
        if (Soul)
        {
            // PlayerStart의 회전(바라보는 방향)을 기준으로 좌우로 간격을 벌려줍니다.
            // i가 0, 1, 2일 때 각각 Y축으로 -150, 0, 150 만큼 떨어지게 설계
            FVector Offset = FVector(0.0f, (i * 150.0f) - 150.0f, 0.0f);
            FVector SpawnLoc = BaseSpawnLoc + BaseSpawnRot.RotateVector(Offset);
            FRotator SpawnRot = BaseSpawnRot;

            UClass* SpawnClass = APlayerBase::StaticClass();
            if (PlayerBaseClass)
            {
                SpawnClass = PlayerBaseClass;
            }

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            APlayerBase* NewBody = GetWorld()->SpawnActor<APlayerBase>(SpawnClass, SpawnLoc, SpawnRot, SpawnParams);

            if (NewBody)
            {
                NewBody->InitializePlayer(Soul);
                ActiveSquadPawns[i] = NewBody;

                if (FirstValidIndex == -1) FirstValidIndex = i;

                //DrawDebugString(GetWorld(), SpawnLoc + FVector(0, 0, 100), FString::Printf(TEXT("Squad_%d"), i), nullptr, FColor::Green, -1.0f);

                // UI 바인딩 지시
                PC->BindPlayerToUI(i, Soul);
            }
        }
    }

    for (APlayerBase* Member : ActiveSquadPawns)
    {
        if (Member) PossessAI(Member);
    }

    if (FirstValidIndex != -1)
    {
        RequestSwitchPlayer(FirstValidIndex);
    }
   /* else
    {
        UE_LOG(LogParadiseSquad, Error, TEXT("❌ [Controller] 스폰된 캐릭터가 아무도 없습니다!"));
    }*/
}


void USquadControlComponent::PossessAI(APlayerBase* TargetCharacter)
{
    if (!TargetCharacter || !SquadAIControllerClass) return;

    AInGameController* PC = GetOwnerPC();

    if (AController* OldCon = TargetCharacter->GetController())
    {
        // [수정] PC(플레이어 컨트롤러)인지 검사
        if (OldCon == PC) return;

        OldCon->UnPossess();
        OldCon->Destroy();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AAIController* NewAI = GetWorld()->SpawnActor<AAIController>(
        SquadAIControllerClass,
        TargetCharacter->GetActorLocation(),
        TargetCharacter->GetActorRotation(),
        SpawnParams
    );

    if (NewAI)
    {
        NewAI->Possess(TargetCharacter);

        if (ASquadAIController* SquadAI = Cast<ASquadAIController>(NewAI))
        {
            // [수정] PC->GetPawn
            SquadAI->SetLeader(PC ? PC->GetPawn() : nullptr);
        }

       //UE_LOG(LogParadiseSquad, Log, TEXT("🤖 [AI] %s에게 AI 컨트롤러가 빙의했습니다."), *TargetCharacter->GetName());
    }
}

bool USquadControlComponent::SwitchToNextSurvivor()
{
    int32 NextAliveIndex = -1;
    int32 SquadSize = ActiveSquadPawns.Num();

    // 현재 인덱스 다음부터 생존 플레이어 탐색
    for (int32 i = 1; i < SquadSize; i++)
    {
        int32 CheckIndex = (CurrentControlledIndex + i) % SquadSize;
        APlayerBase* Candidate = ActiveSquadPawns[CheckIndex];

        if (IsValid(Candidate) && !Candidate->IsDead())
        {
            NextAliveIndex = CheckIndex;
            break;
        }
    }

    if (NextAliveIndex != -1)
    {
        RequestSwitchPlayer(NextAliveIndex);
        return true; // 생존자 찾음
    }

    bIsSquadWipedOut = true; // 전멸 처리

    return false; // 전멸
}
