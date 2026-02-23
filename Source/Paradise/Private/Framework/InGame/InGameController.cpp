// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/InGame/InGameController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/Core/ParadiseGameInstance.h" //디버그함수때문에 추가 이후 삭제
#include "Framework/System/SquadSubsystem.h" //디버그함수때문에 추가 이후 삭제
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "AIController.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/Player/PlayerData.h"
#include "Kismet/GameplayStatics.h"
#include "UI/HUD/Ingame/InGameHUDWidget.h"
#include "UI/Panel/Ingame/PartyStatusPanel.h"
#include "Blueprint/UserWidget.h"

void AInGameController::BeginPlay()
{
	Super::BeginPlay();

    //입력 매핑 컨텍스트 연결
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    InitializeOverviewCamera();

    // [추가] 26/02/04, 담당자: 최지원, [UI 생성] 로컬 플레이어인 경우에만 HUD 생성 (서버/AI 제외)
    
    GetOrCreateInGameHUD();
}
void AInGameController::InitializeOverviewCamera()
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

void AInGameController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Enhanced Input 바인딩
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // 1번 키 -> 인덱스 0
        if (IA_SwitchHero1)
            EnhancedInputComponent->BindAction(IA_SwitchHero1, ETriggerEvent::Triggered, this, &AInGameController::OnInputSwitchHero1);

        // 2번 키 -> 인덱스 1
        if (IA_SwitchHero2)
            EnhancedInputComponent->BindAction(IA_SwitchHero2, ETriggerEvent::Triggered, this, &AInGameController::OnInputSwitchHero2);

        // 3번 키 -> 인덱스 2
        if (IA_SwitchHero3)
            EnhancedInputComponent->BindAction(IA_SwitchHero3, ETriggerEvent::Triggered, this, &AInGameController::OnInputSwitchHero3);
    }

}

void AInGameController::SetAutoBattleMode(bool bEnable)
{
    bIsAutoMode = bEnable;

 
    UE_LOG(LogTemp, Warning, TEXT("🤖 [Controller] 자동 전투 모드: %s"), bEnable ? TEXT("ON") : TEXT("OFF"));
    UpdateCameraSystem(); //카메라시점 전체시점으로 변경

    //TODO: 이 아래에 자동모드 AI 전환 함수 구현
}

void AInGameController::RequestSwitchPlayer(int32 PlayerIndex)
{
    if (!ActiveSquadPawns.IsValidIndex(PlayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Controller] 잘못된 인덱스 요청: %d"), PlayerIndex);
        return;
    }

    APlayerBase* NewPlayer = ActiveSquadPawns[PlayerIndex];
    APlayerBase* OldPlayer = Cast<APlayerBase>(GetPawn());

    //이미 조종 중이거나 대상이 없으면 리턴
    if (!NewPlayer || NewPlayer == OldPlayer) return;
    //죽어있는 플레이어 Base는 리턴
    if (NewPlayer && NewPlayer->IsDead()) return;


    //요청된 캐릭터에 AI가 붙어있었다면 제거
    if (AController* NewPawnController = NewPlayer->GetController())
    {
        // AI 컨트롤러라면 제거
        if (NewPawnController != this)
        {
            NewPawnController->UnPossess();
            NewPawnController->Destroy();
        }
    }

    //요청된 캐릭터로 빙의
    Possess(NewPlayer);
    CurrentControlledIndex = PlayerIndex;

    //이전캐릭터에 AI 주입
    if (OldPlayer)
    {
        PossessAI(OldPlayer);
    }

    // 로그
    FString Msg = FString::Printf(TEXT("Switch -> Hero %d"), PlayerIndex + 1);
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, Msg);

    UE_LOG(LogTemp, Warning, TEXT("🔄 [Controller] 캐릭터 교체 완료 (%s -> %s)"),
        OldPlayer ? *OldPlayer->GetName() : TEXT("None"), // <-- 수정됨
        *NewPlayer->GetName());
        
    UpdateCameraSystem();
}

void AInGameController::RespawnSquadPlayer(int32 PlayerIndex)
{
    //유효성 검사
    AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
    if (!PS || !ActiveSquadPawns.IsValidIndex(PlayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Respawn] 잘못된 인덱스거나 PS가 없습니다."));
        return;
    }

    APlayerData* Soul = PS->GetSquadMemberData(PlayerIndex);
    if (!Soul) return;

    // 이미 살아있는지 확인
    // ActiveSquadPawns[MemberIndex]가 유효하고, IsDead()가 false라면 리턴
    if (ActiveSquadPawns[PlayerIndex] && !ActiveSquadPawns[PlayerIndex]->IsDead())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [Respawn] 해당 멤버는 이미 살아있습니다."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("✨ [Respawn] 멤버 %d (%s) 부활 시퀀스 시작!"), PlayerIndex, *Soul->GetName());

    //소환 위치 결정 (현재 조종 중인 캐릭터 주변)
    //위치 수정 예정
    FVector PlayerSpawnLocation = FVector::ZeroVector;
    FRotator PlayerSpawnRotation = FRotator::ZeroRotator;

    if (APawn* LeaderPawn = GetPawn())
    {
        // 내 캐릭터의 뒤쪽 1.5미터, 위로 0.5미터 지점
        PlayerSpawnLocation = LeaderPawn->GetActorLocation() - (LeaderPawn->GetActorForwardVector() * 150.0f) + FVector(0, 0, 50.0f);
        PlayerSpawnRotation = LeaderPawn->GetActorRotation();
    }
    else
    {
        // 전멸 상태라면 PlayerStart 위치 등 기본값 사용
        SpawnLocation = FVector(0, 0, 200.0f);
    }

    //육체(Body) 스폰
    UClass* SpawnClass = nullptr;

    if (TestPlayerClass) {
        SpawnClass = TestPlayerClass;
    }
    else {
        SpawnClass = APlayerBase::StaticClass();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APlayerBase* NewBody = GetWorld()->SpawnActor<APlayerBase>(SpawnClass, PlayerSpawnLocation, PlayerSpawnRotation, SpawnParams);

    if (NewBody)
    {
        //데이터 연동 (영혼 주입)
        NewBody->InitializePlayer(Soul);

        //관리 목록 갱신 (죽은 시체 포인터를 새 몸으로 교체)
        ActiveSquadPawns[PlayerIndex] = NewBody;

        //상태 초기화
        Soul->bIsDead = false; // PlayerData에 별도 Setter가 있다면 그걸 사용하세요.

        //전멸상태거나 , 조종중인 Player이 없을때
        if (bIsSquadWipedOut || GetPawn() == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("✨ [Respawn] 전멸 위기에서 %s 부활! 제어권을 획득합니다."), *NewBody->GetName());

            // 전멸 플래그 해제
            bIsSquadWipedOut = false;

            //Possess 및 인덱스 초기화
            Possess(NewBody);
            CurrentControlledIndex = PlayerIndex;

            // 카메라 시점을 이 캐릭터로 갱신
            UpdateCameraSystem();
        }
        else
        {
            // 이미 다른 캐릭터를 조종 중이라면, 부활한 캐릭터는 AI에게 맡김
            if (PlayerIndex != CurrentControlledIndex)
            {
                PossessAI(NewBody);
            }
        }
    }
}

void AInGameController::OnPlayerDied(APlayerBase* DeadPlayer)
{
    bool bIsMyCharacter = false;
    if (ActiveSquadPawns.IsValidIndex(CurrentControlledIndex))
    {
        bIsMyCharacter = (ActiveSquadPawns[CurrentControlledIndex] == DeadPlayer);
    }

    if (bIsMyCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("🚨 [Controller] 플레이어 사망 확인! (Index: %d) -> 다음 생존자 탐색 시작"), CurrentControlledIndex);

        int32 NextAliveIndex = -1;
        int32 SquadSize = ActiveSquadPawns.Num();

        // 현재 인덱스 다음부터 한 바퀴 돌면서 생존 플레이어 탐색
        for (int32 i = 1; i < SquadSize; i++)
        {
            int32 CheckIndex = (CurrentControlledIndex + i) % SquadSize;
            APlayerBase* Candidate = ActiveSquadPawns[CheckIndex];

            //생존 플레이어가 있으면  (Candidate가 있고, 죽지 않았어야 함)
            if (Candidate && !Candidate->IsDead())
            {
                NextAliveIndex = CheckIndex;
                break;
            }
        }

        // 생존 플레이어가 있으면 교체, 없으면 게임 오버
        if (NextAliveIndex != -1)
        {
            // 바로 교체 요청
            RequestSwitchPlayer(NextAliveIndex);
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("💀 [Controller] 모든 스쿼드 멤버가 사망했습니다."));

            if (PlayerCameraManager)
            {
                LastDeathLocation = PlayerCameraManager->GetCameraLocation();
                LastDeathRotation = PlayerCameraManager->GetCameraRotation();
            }
            else {
                UE_LOG(LogTemp, Error, TEXT("💀 [Controller] PlayerCameraManager가 없습니다."));
            }

            bIsSquadWipedOut = true;

            //타이밍상 다음 틱에 UpdateCameraSystem 실행
            GetWorld()->GetTimerManager().SetTimerForNextTick(
                this, 
                &AInGameController::UpdateCameraSystem
            );
          
        }
    }
    else
    {
        // (AI 동료가 죽은 경우)
        UE_LOG(LogTemp, Warning, TEXT("🤖 [Controller] 동료(AI)가 사망했습니다."));
    }
}

void AInGameController::UpdateCameraSystem()
{
    if (PlayerCameraManager)
    {
        if (PlayerCameraManager->BlendTimeToGo > 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("⏳ [Camera] 엔진이 블렌딩 처리 중입니다. (남은 시간: %.2f)"), PlayerCameraManager->BlendTimeToGo);
            return;
        }
    }

    // 우선순위 1: 전멸했거나 자동 모드일 때 -> Overview 카메라
    if ((bIsSquadWipedOut || bIsAutoMode) && OverviewCameraActor)
    {
        if (bIsSquadWipedOut)
        {
            //컨트롤러(나 자신)를 마지막 사망 위치로 이동시킴
            ClientSetLocation(LastDeathLocation, LastDeathRotation);

            //뷰 타겟을 this 컨트롤러로 즉시 고정
            //전멸시 카메라 시점 이상한것 해결
            SetViewTarget(this);
        }

        //Overview로 부드럽게 이동
        if (GetViewTarget() != OverviewCameraActor)
        {
            SetViewTargetWithBlend(OverviewCameraActor, 1.5f, VTBlend_Cubic);
            UE_LOG(LogTemp, Log, TEXT("📷 [Camera] Overview 모드로 전환 (From Death Pos)"));
        }
    }
    // 우선순위 2: 조종 가능한 캐릭터가 있을 때 -> 캐릭터 카메라
    else if (GetPawn())
    {
        if (GetViewTarget() != GetPawn())
        {
            SetViewTargetWithBlend(GetPawn(), CameraBlendTime, VTBlend_Cubic);
            UE_LOG(LogTemp, Log, TEXT("📷 [Camera] 캐릭터 모드로 복귀"));
        }
    }
}

void AInGameController::InitializeSquadPawns()
{
    AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
    if (!PS) return;

    UE_LOG(LogTemp, Warning, TEXT("🎮 [Controller] 육체(Pawn) 소환 시작..."));

    for (int32 i = 0; i < PS->GetSquadSize(); i++)
    {
        APlayerData* Soul = PS->GetSquadMemberData(i);
        if (Soul)
        {
            FVector SpawnLoc = FVector(0, i * 200.0f, 100.0f);
            FRotator SpawnRot = FRotator::ZeroRotator;

            //에디터에서 지정한 TestPlayerClass가 있으면 그걸 쓰고, 없으면 기본 C++ 클래스 사용
            UClass* SpawnClass = nullptr;

            if (TestPlayerClass) {
                SpawnClass = TestPlayerClass;
            }
            else {
                SpawnClass = APlayerBase::StaticClass();
            }

            if (!TestPlayerClass)
            {
                UE_LOG(LogTemp, Error, TEXT("⚠️ TestPlayerClass가 설정되지 않았습니다! 입력이 작동하지 않을 수 있습니다."));
            }

            APlayerBase* NewBody = GetWorld()->SpawnActor<APlayerBase>(SpawnClass, SpawnLoc, SpawnRot);

            if (NewBody)
            {
                NewBody->InitializePlayer(Soul);
                ActiveSquadPawns.Add(NewBody);
                DrawDebugString(GetWorld(), SpawnLoc + FVector(0, 0, 100), FString::Printf(TEXT("Squad_%d"), i), nullptr, FColor::Green, -1.0f);

                BindPlayerToUI(i, Soul);
            }
        }
    }

    for (APlayerBase* Member : ActiveSquadPawns)
    {
        if (Member)
        {
            //일단 전부 AI컨트롤러 Possess
            PossessAI(Member);
        }
    }

    //첫번째 캐릭터에 변경요청
    RequestSwitchPlayer(0);
}

void AInGameController::PossessAI(APlayerBase* TargetCharacter)
{
    if (!TargetCharacter || !SquadAIControllerClass) return;

    //기존 컨트롤러 정리
    if (AController* OldCon = TargetCharacter->GetController())
    {
        //만약 (PlayerController)라면 건드리지 않음
        if (OldCon == this) return;

        OldCon->UnPossess();
        OldCon->Destroy(); // 기존 AI 삭제
    }

    //AI 컨트롤러 스폰
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    //많은 양의 스폰액터가 아니기때문에 오브젝트 풀링 미적용예정
    AAIController* NewAI = GetWorld()->SpawnActor<AAIController>(
        SquadAIControllerClass,
        TargetCharacter->GetActorLocation(),
        TargetCharacter->GetActorRotation(),
        SpawnParams
    );

    if (NewAI)
    {
        //빙의 (OnPossess가 호출되면서 비헤이비어 트리가 실행됨)
        NewAI->Possess(TargetCharacter);
        UE_LOG(LogTemp, Log, TEXT("🤖 [AI] %s에게 AI 컨트롤러가 빙의했습니다."), *TargetCharacter->GetName());
    }
}

void AInGameController::BindPlayerToUI(int32 PlayerIndex, APlayerData* InPlayerData)
{
    UE_LOG(LogTemp, Error, TEXT("================ [UI 연동 추적 시작: %d번 슬롯] ================"), PlayerIndex);

    UInGameHUDWidget* HUD = GetOrCreateInGameHUD();

    // 1. HUD 인스턴스가 있는지 확인
    if (!InGameHUDInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ 추적 실패: InGameHUDInstance가 NULL입니다!"));
        UE_LOG(LogTemp, Error, TEXT("   -> 원인: BP_InGameController의 'InGameHUDClass'에 위젯이 안 채워져 있거나, 생성에 실패했습니다."));
        return;
    }

    // 2. 플레이어 데이터가 있는지 확인
    if (!InPlayerData)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ 추적 실패: InPlayerData가 NULL입니다!"));
        return;
    }

    // 3. 파티 패널이 있는지 확인
    UPartyStatusPanel* PartyPanel = InGameHUDInstance->GetPartyStatusPanel();
    if (!PartyPanel)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ 추적 실패: PartyStatusPanel이 NULL입니다!"));
        UE_LOG(LogTemp, Error, TEXT("   -> 원인: WBP_InGameHUD 안에 파티 패널이 없거나, 변수 이름(BindWidget)이 틀렸습니다."));
        return;
    }

    // 4. 통과 완료! 
    UE_LOG(LogTemp, Warning, TEXT("✅ 추적 통과: 모든 패널이 정상! PartyPanel에게 바인딩을 명령합니다."));

    PartyPanel->BindMemberASC(PlayerIndex, InPlayerData->GetAbilitySystemComponent());
    PartyPanel->InitializeMember(PlayerIndex, InPlayerData->CharacterID);

    UE_LOG(LogTemp, Error, TEXT("=================================================================="));

    //if (!InGameHUDInstance || !InPlayerData) return;

    //if (UPartyStatusPanel* PartyPanel = InGameHUDInstance->GetPartyStatusPanel())
    //{
    //    // 1. ASC(심장)를 UI에 연동하여 체력/마나가 실시간으로 움직이게 합니다.
    //    PartyPanel->BindMemberASC(PlayerIndex, InPlayerData->GetAbilitySystemComponent());

    //    // 2. 캐릭터 ID를 넘겨주어 데이터 테이블에서 초상화 이미지를 가져오게 합니다.
    //    PartyPanel->InitializeMember(PlayerIndex, InPlayerData->CharacterID);

    //    UE_LOG(LogTemp, Log, TEXT("[Controller] %d번 파티원 UI(초상화 및 ASC) 연동 완료!"), PlayerIndex);
    //}
}

UInGameHUDWidget* AInGameController::GetOrCreateInGameHUD()
{
    // 1. 이미 존재한다면 그대로 반환
    if (InGameHUDInstance)
    {
        return InGameHUDInstance;
    }

    // 2. 타이밍 이슈로 아직 없다면 즉시 생성(Lazy Init)
    if (IsLocalController() && InGameHUDClass)
    {
        InGameHUDInstance = CreateWidget<UInGameHUDWidget>(this, InGameHUDClass);
        if (InGameHUDInstance)
        {
            InGameHUDInstance->AddToViewport();
            InGameHUDInstance->InitializeHUD();
            UE_LOG(LogTemp, Warning, TEXT("⚡ [Controller] UI 지연 생성(Lazy Init) 발동! InGameHUD를 즉시 띄웠습니다."));
        }
    }
    else if (!InGameHUDClass && IsLocalController())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Controller] InGameHUDClass가 비어있습니다. BP_InGameController에서 클래스를 할당하세요."));
    }

    return InGameHUDInstance;
}

void AInGameController::OnInputSwitchHero1(const FInputActionValue& Value)
{
    //입력 액션 바인딩 함수 후에 UI 모바일 버튼으로 바인딩예정
    RequestSwitchPlayer(0);
}

void AInGameController::OnInputSwitchHero2(const FInputActionValue& Value)
{
    //입력 액션 바인딩 함수 후에 UI 모바일 버튼으로 바인딩예정
    RequestSwitchPlayer(1);
}

void AInGameController::OnInputSwitchHero3(const FInputActionValue& Value)
{
    //입력 액션 바인딩 함수 후에 UI 모바일 버튼으로 바인딩예정
    RequestSwitchPlayer(2);
}


