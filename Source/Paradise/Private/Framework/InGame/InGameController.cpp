// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/InGame/InGameController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/Core/ParadiseCameraManager.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/Player/PlayerData.h"
#include "Kismet/GameplayStatics.h"

#include "Components/AutoCombatComponent.h"
#include "Components/SquadControlComponent.h"
#include "Components/FamiliarSummonComponent.h"
#include "Components/EquipmentComponent.h"
#include "Components/UltimateEffectComponent.h"

#include "AIController.h"
#include "AI/Squad/SquadAIController.h"

#include "Camera/CameraComponent.h"

#include "Framework/InGame/InGameGameMode.h"//디버그치트함수때문에 추가 이후 삭제
#include "GAS/Attributes/BaseAttributeSet.h"//디버그치트함수때문에 추가 이후 삭제
#include "AbilitySystemComponent.h"//데미지 주는 치트함수 때문에 추가 이후 삭제 예정
#include "GameplayEffect.h" //데미지 주는 치트함수 때문에 추가 이후 삭제 예정

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"

#include "UI/HUD/Ingame/InGameHUDWidget.h"
#include "UI/Panel/Ingame/PartyStatusPanel.h"
#include "UI/Panel/Ingame/ActionControlPanel.h"
#include "UI/Panel/Ingame/SummonControlPanel.h"
#include "UI/Widgets/Setting/SettingsPopupWidget.h"
#include "Blueprint/UserWidget.h"

AInGameController::AInGameController()
{
    AutoCombatComponent = CreateDefaultSubobject<UAutoCombatComponent>(TEXT("AutoCombatComponent"));
    SquadControlComponent = CreateDefaultSubobject<USquadControlComponent>(TEXT("SquadControlComponent"));
    UltimateEffectComponent = CreateDefaultSubobject<UUltimateEffectComponent>(TEXT("UltimateEffectComponent"));

    PlayerCameraManagerClass = AParadiseCameraManager::StaticClass();
}

void AInGameController::BeginPlay()
{
	Super::BeginPlay();

    //입력 매핑 컨텍스트 연결
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
           // UE_LOG(LogTemp, Warning, TEXT("[Input] MappingContext 등록 완료"));
        }
    }

    CachedGameInstance = Cast<UParadiseGameInstance>(GetGameInstance());
    CachedPlayerState = GetPlayerState<AInGamePlayerState>();

    // [추가] 26/02/04, 담당자: 최지원, [UI 생성] 로컬 플레이어인 경우에만 HUD 생성 (서버/AI 제외)
    
    GetOrCreateInGameHUD();
}


void AInGameController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Enhanced Input 바인딩
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // 1번 키 -> 인덱스 0
        if (IA_SwitchHero1) EnhancedInputComponent->BindAction(IA_SwitchHero1, ETriggerEvent::Triggered, this, &AInGameController::OnInputSwitchHero1);

        // 2번 키 -> 인덱스 1
        if (IA_SwitchHero2) EnhancedInputComponent->BindAction(IA_SwitchHero2, ETriggerEvent::Triggered, this, &AInGameController::OnInputSwitchHero2);

        // 3번 키 -> 인덱스 2
        if (IA_SwitchHero3) EnhancedInputComponent->BindAction(IA_SwitchHero3, ETriggerEvent::Triggered, this, &AInGameController::OnInputSwitchHero3);
        
        if (IA_Move)EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AInGameController::OnInputMove);
        // 퍼밀리어 소환 슬롯 바인딩
        if (IA_SummonSlot1) EnhancedInputComponent->BindAction(IA_SummonSlot1, ETriggerEvent::Started, this, &AInGameController::OnInputSummonSlot1);
        if (IA_SummonSlot2) EnhancedInputComponent->BindAction(IA_SummonSlot2, ETriggerEvent::Started, this, &AInGameController::OnInputSummonSlot2);
        if (IA_SummonSlot3) EnhancedInputComponent->BindAction(IA_SummonSlot3, ETriggerEvent::Started, this, &AInGameController::OnInputSummonSlot3);
        if (IA_SummonSlot4) EnhancedInputComponent->BindAction(IA_SummonSlot4, ETriggerEvent::Started, this, &AInGameController::OnInputSummonSlot4);
        if (IA_SummonSlot5) EnhancedInputComponent->BindAction(IA_SummonSlot5, ETriggerEvent::Started, this, &AInGameController::OnInputSummonSlot5);

        if (IA_Attack) EnhancedInputComponent->BindAction(IA_Attack, ETriggerEvent::Started, this, &AInGameController::OnInputAttack);
        if (IA_Skill) EnhancedInputComponent->BindAction(IA_Skill, ETriggerEvent::Started, this, &AInGameController::OnInputSkill);
        if (IA_Ultimate) EnhancedInputComponent->BindAction(IA_Ultimate, ETriggerEvent::Started, this, &AInGameController::OnInputUltimate);

        if (IA_OpenSettings) EnhancedInputComponent->BindAction(IA_OpenSettings, ETriggerEvent::Started, this, &AInGameController::OnInputOpenSettings);
    }

}

void AInGameController::OnInputMove(const FInputActionValue& Value)
{
    bShowMouseCursor = false;

    APlayerBase* CurrentPawn = Cast<APlayerBase>(GetPawn());
    if (!CurrentPawn || !CurrentPawn->CanMove()) return;

    const FVector2D MoveInput = Value.Get<FVector2D>();

    if (UCameraComponent* CameraComp = CurrentPawn->FindComponentByClass<UCameraComponent>())
    {
        const FRotator YawRot(0, CameraComp->GetComponentRotation().Yaw, 0);
        const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
        const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

        CurrentPawn->AddMovementInput(Forward, MoveInput.Y);
        CurrentPawn->AddMovementInput(Right, MoveInput.X);
    }
}

void AInGameController::OnInputSummonSlot1() { RequestFamiliarSummon(0); }
void AInGameController::OnInputSummonSlot2() { RequestFamiliarSummon(1); }
void AInGameController::OnInputSummonSlot3() { RequestFamiliarSummon(2); }
void AInGameController::OnInputSummonSlot4() { RequestFamiliarSummon(3); }
void AInGameController::OnInputSummonSlot5() { RequestFamiliarSummon(4); }

void AInGameController::OnInputAttack(const FInputActionValue& Value)
{
    bShowMouseCursor = false;

    if (UInGameHUDWidget* HUD = GetOrCreateInGameHUD())
    {
        if (UActionControlPanel* ActionPanel = HUD->GetActionControlPanel())
        {
            ActionPanel->KeyboardAttack();
        }
    }
}

void AInGameController::OnInputSkill(const FInputActionValue& Value)
{
    bShowMouseCursor = false;

    if (UInGameHUDWidget* HUD = GetOrCreateInGameHUD())
    {
        if (UActionControlPanel* ActionPanel = HUD->GetActionControlPanel())
        {
            ActionPanel->KeyboardSkill();
        }
    }
}

void AInGameController::OnInputUltimate(const FInputActionValue& Value)
{
    bShowMouseCursor = true;

    if (UInGameHUDWidget* HUD = GetOrCreateInGameHUD())
    {
        if (UActionControlPanel* ActionPanel = HUD->GetActionControlPanel())
        {
            ActionPanel->KeyboardUltimate();
        }
    }
}

void AInGameController::OnInputOpenSettings(const FInputActionValue& Value)
{
    // 설정창 열 때 커서 표시
    bShowMouseCursor = true;

    if (UInGameHUDWidget* HUD = GetOrCreateInGameHUD())
    {
        if (USettingsPopupWidget* SettingsPopup = HUD->GetSettingsPopupInstance())
        {
            SettingsPopup->OpenSettings();
        }
    }
}

void AInGameController::RequestFamiliarSummon(int32 SlotIndex)
{

    if (UInGameHUDWidget* HUD = GetOrCreateInGameHUD())
    {
        if (USummonControlPanel* SummonPanel = HUD->GetSummonControlPanel()) // 패널 접근자 필요
        {
            // UI 슬롯 클릭 이벤트로 연결된 함수를 직접 호출하여 완벽하게 우회 실행합니다.
            SummonPanel->HandleSlotClickRequest(SlotIndex);
        }
    }
}

void AInGameController::OnPlayerDied(APlayerBase* DeadPlayer)
{
    if (!SquadControlComponent) return;

    bool bIsMyCharacter = false;
    auto ActiveSquadPawns = SquadControlComponent->GetActiveSquadPawns();
    int32 CurrentControlledIndex = SquadControlComponent->GetCurrentControlledIndex();

    if (ActiveSquadPawns.IsValidIndex(CurrentControlledIndex))
    {
        bIsMyCharacter = (ActiveSquadPawns[CurrentControlledIndex] == DeadPlayer);
    }

    if (bIsMyCharacter)
    {
       // UE_LOG(LogTemp, Warning, TEXT("🚨 [Controller] 플레이어 사망 확인! -> 다음 생존자 탐색 시작"));

        //스쿼드 관리 컴포넌트에서 처리
        bool bHasSurvivor = SquadControlComponent->SwitchToNextSurvivor();

        if (!bHasSurvivor)
        {
            //UE_LOG(LogTemp, Error, TEXT("💀 [Controller] 모든 스쿼드 멤버가 사망했습니다."));

            if (AParadiseCameraManager* CamManager = Cast<AParadiseCameraManager>(PlayerCameraManager))
            {
                // 카메라 매니저의 변수에 현재 카메라 위치를 저장
                CamManager->LastDeathLocation = CamManager->GetCameraLocation();
                CamManager->LastDeathRotation = CamManager->GetCameraRotation();

                // 다음 틱에 카메라 매니저의 UpdateCameraSystem 함수를 실행하도록 타이머 설정
                GetWorld()->GetTimerManager().SetTimerForNextTick(CamManager, &AParadiseCameraManager::UpdateCameraSystem);
            }
        }
    }
    /*else
    {
        UE_LOG(LogTemp, Warning, TEXT("🤖 [Controller] 동료(AI)가 사망했습니다."));
    }*/

    //죽은 캐릭터 덱 풀에 넣기
    int32 DeadIndex = ActiveSquadPawns.Find(DeadPlayer);
    if (DeadIndex != INDEX_NONE && CachedPlayerState.IsValid())
    {
        if (UFamiliarSummonComponent* SummonComp = CachedPlayerState->FindComponentByClass<UFamiliarSummonComponent>())
        {
            // 기본값 설정 (혹시 모를 데이터 누락 대비)
            int32 RespawnCost = 30;
            TSoftObjectPtr<UTexture2D> CharacterIcon = nullptr;

            // 죽은 캐릭터의 고유 데이터 가져오기
            if (APlayerData* Soul = CachedPlayerState->GetSquadMemberData(DeadIndex))
            {
                FName CharID = Soul->CharacterID;

                if (CachedGameInstance.IsValid())
                {
                    //캐릭터 스탯 테이블에서 SummonCost 가져오기
                    if (FCharacterStats* Stats = CachedGameInstance->GetDataTableRow<FCharacterStats>(CachedGameInstance->CharacterStatsDataTable, CharID))
                    {
                        RespawnCost = Stats->SummonCost;
                    }

                    //캐릭터 에셋 테이블에서 FaceIcon 가져오기
                    if (FCharacterAssets* Assets = CachedGameInstance->GetDataTableRow<FCharacterAssets>(CachedGameInstance->CharacterAssetsDataTable, CharID))
                    {
                        CharacterIcon = Assets->FaceIcon;
                    }
                }
            }

            //수집된 데이터를 컴포넌트에 주입
            SummonComp->InjectCharacterRespawnCard(DeadIndex, RespawnCost, CharacterIcon);

            //UE_LOG(LogTemp, Log, TEXT("💀 [Controller] 캐릭터(%d번) 사망: 부활 카드 생성 (비용: %d)"), DeadIndex, RespawnCost);
        }
    }
}

void AInGameController::BindPlayerToUI(int32 PlayerIndex, APlayerData* InPlayerData)
{
    //UE_LOG(LogTemp, Error, TEXT("================ [UI 연동 추적 시작: %d번 슬롯] ================"), PlayerIndex);

    UInGameHUDWidget* HUD = GetOrCreateInGameHUD();

    // 1. HUD 인스턴스가 있는지 확인
    if (!InGameHUDInstance)
    {
        //UE_LOG(LogTemp, Error, TEXT("❌ 추적 실패: InGameHUDInstance가 NULL입니다!"));
        //UE_LOG(LogTemp, Error, TEXT("   -> 원인: BP_InGameController의 'InGameHUDClass'에 위젯이 안 채워져 있거나, 생성에 실패했습니다."));
        return;
    }

    // 2. 플레이어 데이터가 있는지 확인
    if (!InPlayerData)
    {
        //UE_LOG(LogTemp, Error, TEXT("❌ 추적 실패: InPlayerData가 NULL입니다!"));
        return;
    }

    // 3. 파티 패널이 있는지 확인
    UPartyStatusPanel* PartyPanel = InGameHUDInstance->GetPartyStatusPanel();
    if (!PartyPanel)
    {
        //UE_LOG(LogTemp, Error, TEXT("❌ 추적 실패: PartyStatusPanel이 NULL입니다!"));
        //UE_LOG(LogTemp, Error, TEXT("   -> 원인: WBP_InGameHUD 안에 파티 패널이 없거나, 변수 이름(BindWidget)이 틀렸습니다."));
        return;
    }

    // 4. 통과 완료! 
    //UE_LOG(LogTemp, Warning, TEXT("✅ 추적 통과: 모든 패널이 정상! PartyPanel에게 바인딩을 명령합니다."));

    PartyPanel->AddPartyMemberUI(InPlayerData->CharacterID, InPlayerData->GetAbilitySystemComponent());

    //UE_LOG(LogTemp, Error, TEXT("=================================================================="));
}

UInGameHUDWidget* AInGameController::GetOrCreateInGameHUD()
{
    // 1. 이미 존재한다면 그대로 반환
    if (InGameHUDInstance) return InGameHUDInstance;

    // 2. 타이밍 이슈로 아직 없다면 즉시 생성(Lazy Init)
    if (IsLocalController() && InGameHUDClass)
    {
        InGameHUDInstance = CreateWidget<UInGameHUDWidget>(this, InGameHUDClass);
        if (InGameHUDInstance)
        {
            InGameHUDInstance->AddToViewport();
            InGameHUDInstance->InitializeHUD();
            
            // OS 기본 커서 완전히 숨김 (소프트웨어 커서로 대체)
            FInputModeGameAndUI GameAndUIMode;
            GameAndUIMode.SetHideCursorDuringCapture(false);
            GameAndUIMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(GameAndUIMode);
            bShowMouseCursor = false;

            // 인게임 진입 시 소프트웨어 커서 표시
            InGameHUDInstance->ShowMouseCursor(true);
        }
    }
    return InGameHUDInstance;
}

void AInGameController::SetActionPanelEnabled(bool bEnabled)
{
    // 1. HUD 인스턴스를 가져옵니다.
    UInGameHUDWidget* HUD = GetOrCreateInGameHUD();
    if (!HUD) return;

    // 2. HUD가 소유한 액션 컨트롤 패널을 가져와 활성화/비활성화합니다.
    if (UActionControlPanel* ActionPanel = HUD->GetActionControlPanel())
    {
        ActionPanel->SetIsEnabled(bEnabled);

        //UE_LOG(LogTemp, Log, TEXT("📱 [UI] 액션 패널 활성화 상태 변경: %s"), bEnabled ? TEXT("True") : TEXT("False"));
    }
}

void AInGameController::OnInputSwitchHero1(const FInputActionValue& Value)
{
    bShowMouseCursor = false;
    //입력 액션 바인딩 함수 후에 UI 모바일 버튼으로 바인딩예정
    if(SquadControlComponent) SquadControlComponent->RequestSwitchPlayer(0);
}

void AInGameController::OnInputSwitchHero2(const FInputActionValue& Value)
{
    bShowMouseCursor = false;
    //입력 액션 바인딩 함수 후에 UI 모바일 버튼으로 바인딩예정
    if(SquadControlComponent) SquadControlComponent->RequestSwitchPlayer(1);
}

void AInGameController::OnInputSwitchHero3(const FInputActionValue& Value)
{
     bShowMouseCursor = false;
    //입력 액션 바인딩 함수 후에 UI 모바일 버튼으로 바인딩예정
    if(SquadControlComponent) SquadControlComponent->RequestSwitchPlayer(2);
}

void AInGameController::ToggleAutoBattleMode(bool bEnable)
{
    if (AutoCombatComponent)
    {
        AutoCombatComponent->SetAutoBattleMode(bEnable);
    }

    if (AParadiseCameraManager* CamManager = Cast<AParadiseCameraManager>(PlayerCameraManager))
    {
        CamManager->UpdateCameraSystem();
    }
}


void AInGameController::CheatStageClear()
{
    if (AInGameGameMode* gamemode = GetWorld()->GetAuthGameMode<AInGameGameMode>())
    {
        gamemode->EndStage(true);
    }
}

void AInGameController::CheatStageFail()
{
    if (AInGameGameMode* gamemode = GetWorld()->GetAuthGameMode<AInGameGameMode>())
    {
        gamemode->EndStage(false);
    }
}

void AInGameController::CheatKillCharacter(int32 PlayerIndex)
{
    if (SquadControlComponent)
    {
        // 유효한 인덱스인지, 해당 슬롯에 캐릭터가 존재하는지 확인
        if (!SquadControlComponent->GetActiveSquadPawns().IsValidIndex(PlayerIndex) || SquadControlComponent->GetActiveSquadPawns()[PlayerIndex] == nullptr)
        {
            //UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 유효하지 않은 인덱스거나 슬롯이 비어있습니다. (요청 인덱스: %d)"), PlayerIndex);
            return;
        }

        APlayerBase* TargetCharacter = SquadControlComponent->GetActiveSquadPawns()[PlayerIndex];

        // 이미 죽은 캐릭터인지 확인
        if (TargetCharacter->IsDead())
        {
            //UE_LOG(LogTemp, Warning, TEXT("⚠️ [Cheat] %d번 캐릭터(%s)는 이미 사망한 상태입니다."), PlayerIndex, *TargetCharacter->GetName());
            return;
        }

        // GAS를 통해 치명적인 데미지 적용
        if (UAbilitySystemComponent* ASC = TargetCharacter->GetAbilitySystemComponent())
        {
            //UE_LOG(LogTemp, Warning, TEXT("💀 [Cheat] %d번 캐릭터(%s)에게 999,999의 데미지를 가합니다!"), PlayerIndex, *TargetCharacter->GetName());

            //일회성(Instant) 게임플레이 이펙트를 생성
            UGameplayEffect* CheatKillGE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("CheatKillGE")));
            CheatKillGE->DurationPolicy = EGameplayEffectDurationType::Instant;

            // IncomingDamage 어트리뷰트에 999999 데미지를 더하는 모디파이어(Modifier) 설정
            FGameplayModifierInfo ModInfo;
            ModInfo.Attribute = UBaseAttributeSet::GetIncomingDamageAttribute();
            ModInfo.ModifierOp = EGameplayModOp::Additive;
            ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(999999.0f));
            CheatKillGE->Modifiers.Add(ModInfo);

            //만든 이펙트를 타겟 캐릭터에게 적용
            ASC->ApplyGameplayEffectToSelf(CheatKillGE, 1.0f, ASC->MakeEffectContext());
        }
    }

    
}

void AInGameController::CheatRespawn(int32 PlayerIndex)
{
    if (SquadControlComponent)
    {
        SquadControlComponent->RespawnSquadPlayer(PlayerIndex);
    }
   
}
