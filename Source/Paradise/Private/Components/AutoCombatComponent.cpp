// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AutoCombatComponent.h"
#include "Framework/InGame/InGameController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Components/SquadControlComponent.h"
#include "Components/EquipmentComponent.h"
#include "Components/UltimateEffectComponent.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/Player/PlayerData.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "Components/FamiliarSummonComponent.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Paradise/Paradise.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UAutoCombatComponent::UAutoCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
    bIsAutoMode = false;
}


void UAutoCombatComponent::SetAutoBattleMode(bool bEnable)
{
    AInGameController* OwnerPC = GetOwnerController();
    if (!OwnerPC) return;

    bIsAutoMode = bEnable;

    //자동전투 모드 변경 델리게이트
    OnAutoBattleStateChanged.Broadcast(bIsAutoMode);

    UE_LOG(LogParadiseAutoCombat, Warning, TEXT("🤖 [Controller] 자동 전투 모드: %s"), bEnable ? TEXT("ON") : TEXT("OFF"));

    UWorld* World = GetWorld();
    if (!World) return;

    if (bIsAutoMode)
    {
        // 자동 소환 및 전투 타이머 시작
        World->GetTimerManager().SetTimer(AutoSummonTimerHandle, this, &UAutoCombatComponent::CheckAndAutoSummon, 0.5f, true);
        World->GetTimerManager().SetTimer(AutoCombatTimerHandle, this, &UAutoCombatComponent::UpdateAutoCombat, 0.2f, true);
    }
    else
    {
        // 수동 모드 시 타이머 클리어 및 이동 중지
        World->GetTimerManager().ClearTimer(AutoSummonTimerHandle);
        World->GetTimerManager().ClearTimer(AutoCombatTimerHandle);
        OwnerPC->StopMovement();
    }
}

void UAutoCombatComponent::CheckAndAutoSummon()
{
    // 자동모드가 아니라면 진행X
    if (!bIsAutoMode) return;

    AInGameController* OwnerPC = GetOwnerController();
    if (!OwnerPC) return;

    // PlayerState 가져오기 (소환 컴포넌트 가져오기위함)
    AInGamePlayerState* PS = OwnerPC->GetPlayerState<AInGamePlayerState>();
    if (!PS) return;

    // PlayerState에 있는 소환 컴포넌트 획득
    UFamiliarSummonComponent* SummonComp = PS->FindComponentByClass<UFamiliarSummonComponent>();
    if (SummonComp)
    {
        //맨 왼쪽(0번 인덱스) 슬롯 구매 요청
        //(내부에서 코스트 체크를 알아서 해주므로, 돈이 없으면 조용히 false를 반환하고 끝납니다.)
        bool bSuccess = SummonComp->RequestPurchase(0);

        // 돈이 모여서 성공적으로 뽑았다면 로그 출력!
        if (bSuccess)
        {
            UE_LOG(LogParadiseAutoCombat, Log, TEXT("✨ [AutoMode] 자동 소환 성공! (가장 왼쪽 슬롯)"));
        }
    }
}

void UAutoCombatComponent::UpdateAutoCombat()
{
    if (!bIsAutoMode) return;

    AInGameController* OwnerPC = GetOwnerController();
    if (!OwnerPC) return;

    APlayerBase* PlayerPawn = Cast<APlayerBase>(OwnerPC->GetPawn());
    if (!PlayerPawn || PlayerPawn->IsDead()) return;

    float AttackRange = GetDynamicAttackRange(PlayerPawn);
    float NearestDist = 999999.0f;
    AActor* TargetEnemy = nullptr;

    if (CurrentTarget.IsValid())
    {
        AUnitBase* TargetUnit = Cast<AUnitBase>(CurrentTarget.Get());

        // 죽지 않았다면 기존 타겟을 그대로 유지하고 거리만 갱신
        if (TargetUnit && !TargetUnit->IsDead())
        {
            TargetEnemy = CurrentTarget.Get();
            NearestDist = FVector::Distance(PlayerPawn->GetActorLocation(), TargetEnemy->GetActorLocation());
        }
        else
        {
            // 타겟이 죽었거나 삭제되었다면 기억을 지움
            CurrentTarget.Reset();
        }
    }

    if (!TargetEnemy)
    {
        //가장 가까운 적 탐색
        TargetEnemy = FindNearestEnemy(PlayerPawn, NearestDist);
        if (TargetEnemy)
        {
            // 새로 찾은 녀석을 다음 프레임을 위해 기억해둠
            CurrentTarget = TargetEnemy;
        }
    }

    //현재 타겟 디버그
    if (TargetEnemy)
    {
        if (GEngine)
        {
            FString DebugMsg = FString::Printf(TEXT("🎯 [오토 타겟]: %s (거리: %.1f / 사거리: %.1f)"), *TargetEnemy->GetName(), NearestDist, AttackRange);
            GEngine->AddOnScreenDebugMessage(1, 0.5f, FColor::Green, DebugMsg);
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(1, 0.5f, FColor::Yellow, TEXT("🔍 [오토 타겟]: 주변에 적 없음 (기지로 이동)"));
        }
    }

    //적이 사거리 안에 들어왔다면 공격
    if (TargetEnemy && NearestDist <= AttackRange)
    {
        //멈추기
        OwnerPC->StopMovement();

        //적을 바라보게함
        FVector LookDir = TargetEnemy->GetActorLocation() - PlayerPawn->GetActorLocation();
        LookDir.Z = 0.f;
        PlayerPawn->SetActorRotation(LookDir.Rotation());

        //우선순위 대로 공격 어빌리티 발동
        ExecutePrioritizedAction(PlayerPawn);
    }
    else
    {
        AActor* MoveTarget = TargetEnemy;

        // 맵에 적이없다면 기지로 이동
        if (!MoveTarget)
        {
            MoveTarget = GetEnemyBase();
        }

        // 이동 목표가 설정되었다면 내비메쉬를 따라 이동
        if (MoveTarget)
        {
            UAIBlueprintHelperLibrary::SimpleMoveToActor(OwnerPC, MoveTarget);
        }
    }
}

void UAutoCombatComponent::ExecutePrioritizedAction(APlayerBase* PlayerPawn)
{
    if (!PlayerPawn) return;

    UAbilitySystemComponent* ASC = PlayerPawn->GetAbilitySystemComponent();
    if (!ASC)
    {
        // ASC가 없으면 그냥 기본 평타 시도
        PlayerPawn->SendAbilityInputToASC(EInputID::Attack, true);
        return;
    }

    //특정 InputID의 스킬이 쿨타임/마나 조건이 되는지(사용 가능한지) 검사
    auto CanUseAbility = [&](EInputID InputID) -> bool
        {
            for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
            {
                if (Spec.InputID == static_cast<int32>(InputID))
                {
                    return Spec.Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get());
                }
            }
            return false;
        };

    //궁극기 사용 가능 여부 확인
    if (CanUseAbility(EInputID::Ultimate))
    {
        PlayerPawn->SendAbilityInputToASC(EInputID::Ultimate, true);
        //자동으로 궁극기를 써도 컨트롤러에게 화면 연출을 틀어라 라고 호출
        if (AInGameController* OwnerPC = GetOwnerController())
        {
            if (UUltimateEffectComponent* UltEffectComp = OwnerPC->GetUltimateEffectComponent())
            {
                UltEffectComp->PlayUltimateEffect(2.5f);
            }
        }
        UE_LOG(LogParadiseAutoCombat, Warning, TEXT("🤖 [AutoCombat] %s - 궁극기 발동!"), *PlayerPawn->GetName());
        return;
    }

    // 2순위: 일반 스킬 사용 가능 여부 확인
    if (CanUseAbility(EInputID::Skill))
    {
        PlayerPawn->SendAbilityInputToASC(EInputID::Skill, true);
        UE_LOG(LogParadiseAutoCombat, Log, TEXT("🤖 [AutoCombat] %s - 스킬 발동!"), *PlayerPawn->GetName());
        return;
    }

    //궁극기, 스킬 모두 안되면 기본 평타
    PlayerPawn->SendAbilityInputToASC(EInputID::Attack, true);
}

AActor* UAutoCombatComponent::FindNearestEnemy(APawn* PlayerPawn, float& OutDistance)
{
    //큰값으로 초기화
    OutDistance = MAX_flt;
    AActor* NearestEnemy = nullptr;

    if (!PlayerPawn || !GetWorld()) return nullptr;

    const FVector PlayerLoc = PlayerPawn->GetActorLocation();

    float SearchRadius = 3000.0f;

    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(PlayerPawn);

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    //오버랩 검사
    bool bHit = GetWorld()->OverlapMultiByObjectType(
        OverlapResults,
        PlayerLoc,
        FQuat::Identity,
        ObjectQueryParams,
        FCollisionShape::MakeSphere(SearchRadius),
        CollisionParams
    );

    //처음 한 번만 태그를 검색
    static const FGameplayTag EnemyTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"));

    for (const FOverlapResult& Result : OverlapResults)
    {
        AUnitBase* Unit = Cast<AUnitBase>(Result.GetActor());

        // 유효하지 않거나, 죽었거나, 적군 태그가 아니라면 넘어가기
        if (!Unit || Unit->IsDead() || !Unit->GetFactionTag().MatchesTag(EnemyTag))
        {
            continue;
        }

        //이후에 남아있는 액터는 살아있는 적
        float DistSq = FVector::DistSquared(PlayerLoc, Unit->GetActorLocation());

        if (DistSq < OutDistance)
        {
            OutDistance = DistSq;
            NearestEnemy = Unit;
        }
    }

    // 최종 반환할 때만 제곱근(루트)을 씌워서 실제 거리로 반환
    if (NearestEnemy)
    {
        OutDistance = FMath::Sqrt(OutDistance);
    }

    return NearestEnemy;
}

AActor* UAutoCombatComponent::GetEnemyBase()
{
    if (CachedEnemyBase.IsValid())
    {
        return CachedEnemyBase.Get();
    }

    //최초 1회 또는 다음 스테이지만 전체 탐색 수행
    TArray<AActor*> FoundBases;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHomeBase::StaticClass(), FoundBases);

    static const FGameplayTag EnemyTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"));

    for (AActor* Base : FoundBases)
    {
        AHomeBase* HomeBase = Cast<AHomeBase>(Base);

        if (HomeBase && HomeBase->GetFactionTag().MatchesTag(EnemyTag))
        {
            // 찾아낸 기지를 저장
            CachedEnemyBase = HomeBase;
            return Base;
        }
    }
    return nullptr;
}

AInGameController* UAutoCombatComponent::GetOwnerController() const
{
    return Cast<AInGameController>(GetOwner());
}

float UAutoCombatComponent::GetDynamicAttackRange(APlayerBase* PlayerPawn)
{
    float DefaultRange = 150.0f; // 데이터를 못 찾았을 때의 기본 사거리
    if (!PlayerPawn) return DefaultRange;

    AInGameController* OwnerPC = GetOwnerController();
    UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance());
    if (!OwnerPC || !GI) return DefaultRange;

    UAbilitySystemComponent* ASC = PlayerPawn->GetAbilitySystemComponent();
    if (!ASC) return DefaultRange;

    // 스킬 사용 가능 여부 검사 (람다)
    auto CanUseAbility = [&](EInputID InputID) -> bool
        {
            for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
            {
                if (Spec.InputID == static_cast<int32>(InputID))
                    return Spec.Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get());
            }
            return false;
        };

    // 캐릭터 데이터(영혼) 가져오기
    AInGamePlayerState* PS = OwnerPC->GetPlayerState<AInGamePlayerState>();
    USquadControlComponent* SquadComp = OwnerPC->GetSquadControlComponent();
    if (!PS || !SquadComp) return DefaultRange;

    APlayerData* Soul = PS->GetSquadMemberData(SquadComp->GetCurrentControlledIndex());
    if (!Soul) return DefaultRange;

    FDataTableRowHandle TargetActionHandle;

    // 1순위: 궁극기 검사
    if (CanUseAbility(EInputID::Ultimate))
    {
        if (const FCharacterStats* CharStats = GI->GetDataTableRow<FCharacterStats>(GI->CharacterStatsDataTable, Soul->CharacterID))
            TargetActionHandle = CharStats->UltimateActionHandle;
    }
    // 2, 3순위: 무기 스킬 및 평타 검사
    else if (UEquipmentComponent* EquipComp = Soul->GetEquipmentComponent())
    {
        FName WeaponID = EquipComp->GetEquippedItemID(EEquipmentSlot::Weapon);
        if (const FWeaponStats* WeaponStats = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, WeaponID))
            TargetActionHandle = CanUseAbility(EInputID::Skill) ? WeaponStats->SkillActionHandle : WeaponStats->BasicAttackActionHandle;
    }

    // 최종 사거리 반환
    if (!TargetActionHandle.IsNull())
    {
        if (const FActionStats* ActionStats = TargetActionHandle.GetRow<FActionStats>(TEXT("AutoCombatRangeLookup")))
        {
            // AI의 헛방 방지를 위해 실제 사거리의 90%를 적용 (버퍼)
            return ActionStats->AttackRange * 0.9f;
        }
    }

    // 🚨 데이터를 찾지 못했을 때만 에러 파악을 위해 경고 로그 출력
    UE_LOG(LogParadiseAutoCombat, Warning, TEXT("⚠️ [AutoCombat] 액션 데이터를 찾지 못했습니다. 기본 사거리(%.1f) 반환."), DefaultRange);
    return DefaultRange;
}