// Fill out your copyright notice in the Description page of Project Settings.

#include "Objects/HomeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "Framework/System/StageSubsystem.h"
#include "Framework/InGame/InGameGameMode.h"
#include "Framework/InGame/InGameGameState.h"
#include "Framework/Core/ParadiseGameInstance.h"

AHomeBase::AHomeBase()
{
	//MaxHP = 500.f;
	//HP = MaxHP;
	//bIsDead = false;

	PrimaryActorTick.bCanEverTick = false;
}

void AHomeBase::BeginPlay()
{
	Super::BeginPlay();

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	// 1. StageSubsystem에서 현재 선택된 StageID 가져오기
	if (UStageSubsystem* StageSys = GI->GetSubsystem<UStageSubsystem>())
	{
		TargetStageID = StageSys->GetSelectedStageID();
	}

	// 2. 팩션 태그 확인 (체력 계산을 위해 순서를 위로 올림)
	FGameplayTag MyTag = GetFactionTag();
	bool bIsEnemy = MyTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy")));
	bool bIsFriendly = MyTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Friendly")));

	// 3. 체력 계산
	float FinalMaxHP = 1000.f; // 기본 체력

	// 🌟 [수정 부분] 적군 기지(Enemy)인 경우에만 엑셀 배율을 적용
	if (bIsEnemy && GI->StatgeStatsDataTable && !TargetStageID.IsNone())
	{
		if (FStageStats* Stats = GI->GetDataTableRow<FStageStats>(GI->StatgeStatsDataTable, TargetStageID))
		{
			// 소수점 지저분함을 방지하기 위해 반올림(RoundToFloat) 처리 추가
			FinalMaxHP = FMath::RoundToFloat(FinalMaxHP * Stats->HomeBaseHPMultiplier);
		}
	}

	// 4. AttributeSet 초기화 (아군은 1000, 적군은 배율 적용된 값으로 세팅됨)
	if (UBaseAttributeSet* BaseSet = Cast<UBaseAttributeSet>(AttributeSet))
	{
		BaseSet->InitMaxHealth(FinalMaxHP);
		BaseSet->InitHealth(FinalMaxHP);

		UE_LOG(LogTemp, Warning, TEXT("🏰 [%s] 성 체력 세팅 완료! MaxHP: %f"), *MyTag.ToString(), FinalMaxHP);
	}

	// 5. 이동 및 물리 설정 (기존과 동일)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		MoveComp->Deactivate();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetSimulatePhysics(false);
		Capsule->SetEnableGravity(false);
		Capsule->SetMobility(EComponentMobility::Stationary);
	}

	// 6. GameState 등록
	if (AInGameGameState* GS = Cast<AInGameGameState>(GetWorld()->GetGameState()))
	{
		if (bIsFriendly)
		{
			GS->RegisterAllyHomeBase(this);
		}
		else if (bIsEnemy)
		{
			GS->RegisterEnemyHomeBase(this);
		}
	}
}

void AHomeBase::Die()
{
    //UE_LOG(LogTemp, Error, TEXT("🚩 [BASE DESTROYED] %s 가 파괴되었습니다!"), *GetName());

    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    AInGameGameMode* GM = Cast<AInGameGameMode>(GetWorld()->GetAuthGameMode());

    if (GM)
    {
        bool bIsVictory = false;

        // 1. UnitBase의 FactionTag 변수를 가져옵니다.
        FGameplayTag MyTag = GetFactionTag();

        // 2. GameplayTag 매칭 검사
        // 적군 태그를 가지고 있다면 (Unit.Faction.Enemy)
        if (MyTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"))))
        {
            bIsVictory = true;
            //UE_LOG(LogTemp, Warning, TEXT("✅ 적군 기지(%s) 파괴 감지: 승리 판정"), *MyTag.ToString());
        }
        // 아군 태그를 가지고 있다면 (Unit.Faction.Friendly)
        else if (MyTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Friendly"))))
        {
            bIsVictory = false;
            //UE_LOG(LogTemp, Warning, TEXT("❌ 아군 기지(%s) 파괴 감지: 패배 판정"), *MyTag.ToString());
        }
        else
        {
            // 못 찾는 경우를 위한 디버깅용
            //UE_LOG(LogTemp, Error, TEXT("⚠️ 기지에 유효한 FactionTag가 없습니다! 현재 태그: %s"), *MyTag.ToString());
        }

        GM->EndStage(bIsVictory);
    }
}