// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AutoCombatComponent.h"
#include "Framework/InGame/InGameController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Characters/Base/PlayerBase.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Objects/HomeBase.h"
#include "Components/FamiliarSummonComponent.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
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


    UE_LOG(LogTemp, Warning, TEXT("🤖 [Controller] 자동 전투 모드: %s"), bEnable ? TEXT("ON") : TEXT("OFF"));

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
            UE_LOG(LogTemp, Log, TEXT("✨ [AutoMode] 자동 소환 성공! (가장 왼쪽 슬롯)"));
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

    float AttackRange = 300.0f; // TODO: 캐릭터 사거리에 맞게 조절 (현재 하드코딩중)
    float NearestDist = 999999.0f;

    //가장 가까운 적 탐색
    AActor* TargetEnemy = FindNearestEnemy(PlayerPawn, NearestDist);

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
        UE_LOG(LogTemp, Warning, TEXT("🤖 [AutoCombat] %s - 궁극기 발동!"), *PlayerPawn->GetName());
        return;
    }

    // 2순위: 일반 스킬 사용 가능 여부 확인
    if (CanUseAbility(EInputID::Skill))
    {
        PlayerPawn->SendAbilityInputToASC(EInputID::Skill, true);
        UE_LOG(LogTemp, Log, TEXT("🤖 [AutoCombat] %s - 스킬 발동!"), *PlayerPawn->GetName());
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

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), FoundActors);

    //처음 한 번만 태그를 검색
    static const FGameplayTag EnemyTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"));
    const FVector PlayerLoc = PlayerPawn->GetActorLocation();

    for (AActor* Actor : FoundActors)
    {
        AUnitBase* Unit = Cast<AUnitBase>(Actor);
        if (Unit && !Unit->IsDead() && Unit->GetFactionTag().MatchesTag(EnemyTag))
        {
            //DistSquared(제곱 거리) 사용
            float DistSq = FVector::DistSquared(PlayerLoc, Unit->GetActorLocation());
            if (DistSq < OutDistance)
            {
                OutDistance = DistSq;
                NearestEnemy = Actor;
            }
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
    TArray<AActor*> FoundBases;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHomeBase::StaticClass(), FoundBases);

    static const FGameplayTag EnemyTag = FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"));

    for (AActor* Base : FoundBases)
    {
        AHomeBase* HomeBase = Cast<AHomeBase>(Base);

        //기지의 Faction확인하여 적기지만 찾음
        if (HomeBase && HomeBase->GetFactionTag().MatchesTag(EnemyTag))
        {
            return Base;
        }
    }
    return nullptr;
}

AInGameController* UAutoCombatComponent::GetOwnerController() const
{
    return Cast<AInGameController>(GetOwner());
}
