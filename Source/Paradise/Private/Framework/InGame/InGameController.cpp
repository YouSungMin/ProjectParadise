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

#include "AIController.h"
#include "AI/Squad/SquadAIController.h"

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
#include "Blueprint/UserWidget.h"

AInGameController::AInGameController()
{
    AutoCombatComponent = CreateDefaultSubobject<UAutoCombatComponent>(TEXT("AutoCombatComponent"));
    SquadControlComponent = CreateDefaultSubobject<USquadControlComponent>(TEXT("SquadControlComponent"));

    PlayerCameraManagerClass = AParadiseCameraManager::StaticClass();
}

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
        UE_LOG(LogTemp, Warning, TEXT("🚨 [Controller] 플레이어 사망 확인! -> 다음 생존자 탐색 시작"));

        //스쿼드 관리 컴포넌트에서 처리
        bool bHasSurvivor = SquadControlComponent->SwitchToNextSurvivor();

        if (!bHasSurvivor)
        {
            UE_LOG(LogTemp, Error, TEXT("💀 [Controller] 모든 스쿼드 멤버가 사망했습니다."));

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
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🤖 [Controller] 동료(AI)가 사망했습니다."));
    }

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

            UE_LOG(LogTemp, Log, TEXT("💀 [Controller] 캐릭터(%d번) 사망: 부활 카드 생성 (비용: %d)"), DeadIndex, RespawnCost);
        }
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
    if(SquadControlComponent) SquadControlComponent->RequestSwitchPlayer(0);
}

void AInGameController::OnInputSwitchHero2(const FInputActionValue& Value)
{
    //입력 액션 바인딩 함수 후에 UI 모바일 버튼으로 바인딩예정
    if(SquadControlComponent) SquadControlComponent->RequestSwitchPlayer(1);
}

void AInGameController::OnInputSwitchHero3(const FInputActionValue& Value)
{
    //입력 액션 바인딩 함수 후에 UI 모바일 버튼으로 바인딩예정
    if(SquadControlComponent) SquadControlComponent->RequestSwitchPlayer(2);
}

void AInGameController::UpdateActionPanelUI(int32 PlayerIndex)
{
    // 🚨 [최적화] 호출 시점에 유효하지 않은 포인터만 선별적으로 캐싱하여 연산 비용 최소화
    if (!CachedGameInstance.IsValid()) CachedGameInstance = Cast<UParadiseGameInstance>(GetGameInstance());
    if (!CachedPlayerState.IsValid()) CachedPlayerState = GetPlayerState<AInGamePlayerState>();
    if (!InGameHUDInstance) GetOrCreateInGameHUD();

    // 방어 코드: 핵심 시스템 포인터 누락 시 즉시 반환
    if (!CachedGameInstance.IsValid() || !CachedPlayerState.IsValid() || !InGameHUDInstance) return;

    // HUD로부터 액션 제어 패널 획득
    UActionControlPanel* ActionPanel = InGameHUDInstance->GetActionControlPanel();
    if (!ActionPanel) return;

    FName CurrentWeaponSkillID = NAME_None;
    FName CurrentUltimateID = NAME_None;

    // [데이터 드리븐] 영혼 데이터(Soul)로부터 장착 및 스탯 정보 추출
    if (APlayerData* Soul = CachedPlayerState->GetSquadMemberData(PlayerIndex))
    {
        // 1. 캐릭터 스탯 테이블로부터 궁극기(Ultimate) ID 획득
        if (const FCharacterStats* CharStats = CachedGameInstance->GetDataTableRow<FCharacterStats>(CachedGameInstance->CharacterStatsDataTable, Soul->CharacterID))
        {
            CurrentUltimateID = CharStats->SkillActionID;
        }

        // 2. 장비 컴포넌트로부터 무기 스킬(Weapon Skill) ID 획득
        if (UEquipmentComponent* EquipComp = Soul->GetEquipmentComponent())
        {
            // 무기 슬롯(Weapon)에 장착된 아이템의 RowName을 가져옵니다.
            const FName EquippedWeaponID = EquipComp->GetEquippedItemID(EEquipmentSlot::Weapon);

            if (EquippedWeaponID != NAME_None)
            {
                // 무기 스탯 테이블(DT_WeaponStats)로부터 해당 무기가 가진 스킬 ID 획득
                if (const FWeaponStats* WeaponStats = CachedGameInstance->GetDataTableRow<FWeaponStats>(CachedGameInstance->WeaponStatsDataTable, EquippedWeaponID))
                {
                    CurrentWeaponSkillID = WeaponStats->SkillActionID;
                }
            }
        }
    }

    // 3. UI 객체에 추출된 데이터 주입 (UI는 데이터의 출처를 알 필요 없음 - SRP 준수)
    ActionPanel->InitActionPanel(CurrentWeaponSkillID, CurrentUltimateID);
    ActionPanel->UpdateTagButtons(PlayerIndex);

    UE_LOG(LogTemp, Log, TEXT("⚔️ [ActionPanel] UI 갱신 성공 (Index: %d)"), PlayerIndex);
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
            UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 유효하지 않은 인덱스거나 슬롯이 비어있습니다. (요청 인덱스: %d)"), PlayerIndex);
            return;
        }

        APlayerBase* TargetCharacter = SquadControlComponent->GetActiveSquadPawns()[PlayerIndex];

        // 이미 죽은 캐릭터인지 확인
        if (TargetCharacter->IsDead())
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ [Cheat] %d번 캐릭터(%s)는 이미 사망한 상태입니다."), PlayerIndex, *TargetCharacter->GetName());
            return;
        }

        // GAS를 통해 치명적인 데미지 적용
        if (UAbilitySystemComponent* ASC = TargetCharacter->GetAbilitySystemComponent())
        {
            UE_LOG(LogTemp, Warning, TEXT("💀 [Cheat] %d번 캐릭터(%s)에게 999,999의 데미지를 가합니다!"), PlayerIndex, *TargetCharacter->GetName());

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
