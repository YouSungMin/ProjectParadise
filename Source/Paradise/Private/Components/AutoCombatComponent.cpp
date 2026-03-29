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
           // UE_LOG(LogParadiseAutoCombat, Log, TEXT("✨ [AutoMode] 자동 소환 성공! (가장 왼쪽 슬롯)"));
        }
    }
}

void UAutoCombatComponent::UpdateAutoCombat()
{
    if (!bIsAutoMode) return;

    AInGameController* OwnerPC = GetOwnerController();
    if (!OwnerPC) return;

    APlayerBase* PlayerPawn = Cast<APlayerBase>(OwnerPC->GetPawn());
    // 캐릭터가 정상이면 조용히 패스! (여기서 안 튕기고 아래로 잘 내려갑니다)
    if (!PlayerPawn || PlayerPawn->IsDead() || !PlayerPawn->CanMove()) return;

    float AttackRange = GetDynamicAttackRange(PlayerPawn);
    float NearestDist = 999999.0f;
    AActor* TargetEnemy = nullptr;

    if (CurrentTarget.IsValid())
    {
        AUnitBase* TargetUnit = Cast<AUnitBase>(CurrentTarget.Get());
        if (TargetUnit && !TargetUnit->IsDead())
        {
            TargetEnemy = CurrentTarget.Get();
            // 중심점 간의 진짜 거리를 구함
            float CenterDist = FVector::Distance(PlayerPawn->GetActorLocation(), TargetEnemy->GetActorLocation());

            // 나와 적의 충돌체(캡슐) 반지름을 가져옴
            float MyRadius = PlayerPawn->GetSimpleCollisionRadius();
            float TargetRadius = TargetEnemy->GetSimpleCollisionRadius();

            // 중심 거리에서 양쪽의 반지름을 빼서 '표면 간의 거리'를 계산 (음수가 되면 0으로 처리)
            NearestDist = FMath::Max(0.0f, CenterDist - MyRadius - TargetRadius);
            //NearestDist = FVector::Distance(PlayerPawn->GetActorLocation(), TargetEnemy->GetActorLocation());
        }
        else
        {
            CurrentTarget.Reset();
        }
    }

    if (!TargetEnemy)
    {
        TargetEnemy = FindNearestEnemy(PlayerPawn, NearestDist);
        if (TargetEnemy) CurrentTarget = TargetEnemy;
    }

    if (TargetEnemy)
    {
        //UE_LOG(LogParadiseAutoCombat, Warning, TEXT("🎯 [오토 타겟]: %s 추적 중! (거리: %.1f / 내 사거리: %.1f)"), *TargetEnemy->GetName(), NearestDist, AttackRange);
    }
    else
    {
        //UE_LOG(LogParadiseAutoCombat, Warning, TEXT("🔍 [오토 타겟]: 주변에 적 없음! 기지로 이동합니다."));
    }

    // 적이 사거리 안에 들어왔다면 공격
    if (TargetEnemy && NearestDist <= AttackRange)
    {
        OwnerPC->StopMovement();

        FVector LookDir = TargetEnemy->GetActorLocation() - PlayerPawn->GetActorLocation();
        LookDir.Z = 0.f;
        PlayerPawn->SetActorRotation(LookDir.Rotation());

        ExecutePrioritizedAction(PlayerPawn);
    }
    else
    {
        AActor* MoveTarget = TargetEnemy ? TargetEnemy : GetEnemyBase();
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
    if (!ASC) return;

    auto CanUseAbility = [&](EInputID InputID) -> bool
    {
        for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
        {
            if (Spec.Ability && Spec.InputID == static_cast<int32>(InputID))
            {
                return Spec.Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get());
            }
        }
        return false;
    };

    // 1순위: 궁극기
    if (CanUseAbility(EInputID::Ultimate))
    {
        UE_LOG(LogParadiseAutoCombat, Warning, TEXT("🔥 [AutoCombat] %s - 궁극기 발동!"), *PlayerPawn->GetName());
        PlayerPawn->SendAbilityInputToASC(EInputID::Ultimate, true);
        PlayerPawn->SendAbilityInputToASC(EInputID::Ultimate, false); // 💡 눌렀다 떼기 필수!

        if (AInGameController* OwnerPC = GetOwnerController())
        {
            bool bIsAutoBattle = OwnerPC->GetAutoCombatComponent() ? OwnerPC->GetAutoCombatComponent()->IsAutoMode() : false;
            if (!bIsAutoBattle)
            {
                if (UUltimateEffectComponent* UltEffectComp = OwnerPC->GetUltimateEffectComponent())
                {
                    UltEffectComp->PlayUltimateEffect(2.5f);
                }
            }
        }
        return;
    }

    // 2순위: 일반 스킬
    if (CanUseAbility(EInputID::Skill))
    {
        UE_LOG(LogParadiseAutoCombat, Warning, TEXT("✨ [AutoCombat] %s - 스킬 발동!"), *PlayerPawn->GetName());
        PlayerPawn->SendAbilityInputToASC(EInputID::Skill, true);
        PlayerPawn->SendAbilityInputToASC(EInputID::Skill, false); // 💡 눌렀다 떼기 필수!
        return;
    }

    // 3순위: 평타 (평타도 쿨타임/조건이 맞을 때만 쏘게 보호막 설치!)
    if (CanUseAbility(EInputID::Attack))
    {
        UE_LOG(LogParadiseAutoCombat, Warning, TEXT("⚔️ [AutoCombat] %s - 평타 발동!"), *PlayerPawn->GetName());
        PlayerPawn->SendAbilityInputToASC(EInputID::Attack, true);
        PlayerPawn->SendAbilityInputToASC(EInputID::Attack, false); // 💡 눌렀다 떼기 필수!
    }
    else
    {
        UE_LOG(LogParadiseAutoCombat, Error, TEXT("😱 [AutoCombat] %s - 사거리에는 들어왔는데, 아무 스킬도 쏠 수 없는 상태입니다! (쿨타임 대기중이거나 코스트 부족)"), *PlayerPawn->GetName());
    }
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
        float CenterDist = FMath::Sqrt(OutDistance);

        float MyRadius = PlayerPawn->GetSimpleCollisionRadius();
        float TargetRadius = NearestEnemy->GetSimpleCollisionRadius();

        OutDistance = FMath::Max(0.0f, CenterDist - MyRadius - TargetRadius);
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

    //데이터를 찾지 못했을 때 경고 로그 출력
    UE_LOG(LogParadiseAutoCombat, Warning, TEXT("⚠️ [AutoCombat] 액션 데이터를 찾지 못했습니다. 기본 사거리(%.1f) 반환."), DefaultRange);
    return DefaultRange;
}