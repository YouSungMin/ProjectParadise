// Fill out your copyright notice in the Description page of Project Settings.

#include "Objects/HomeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Framework/InGame/InGameGameMode.h"
#include "Framework/InGame/InGameGameState.h"

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

    if (AInGameGameState* GS = Cast<AInGameGameState>(GetWorld()->GetGameState()))
    {
        FGameplayTag MyTag = GetFactionTag();
        if (MyTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Friendly"))))
        {
            GS->RegisterAllyHomeBase(this);
        }
        else if (MyTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Unit.Faction.Enemy"))))
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