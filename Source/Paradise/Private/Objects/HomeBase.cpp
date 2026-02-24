// Fill out your copyright notice in the Description page of Project Settings.

#include "Objects/HomeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Framework/InGame/InGameGameMode.h"

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
}

void AHomeBase::Die()
{
    UE_LOG(LogTemp, Error, TEXT("🚩 [BASE DESTROYED] %s 가 파괴되었습니다!"), *GetName());

    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    AInGameGameMode* GM = Cast<AInGameGameMode>(GetWorld()->GetAuthGameMode());

    if (GM)
    {
        // 1. 승리 여부를 결정할 변수 초기화
        bool bIsVictory = false;

        // 2. 태그를 직접적으로 체크
        if (ActorHasTag(FName("Unit.Faction.Enemy")))
        {
            // 적군 기지가 파괴되었으므로 승리
            bIsVictory = true;
            UE_LOG(LogTemp, Warning, TEXT("✅ 적군 기지 파괴 감지: 승리 판정"));
        }
        else if (ActorHasTag(FName("Unit.Faction.Friendly")))
        {
            // 아군 기지가 파괴되었으므로 패배
            bIsVictory = false;
            UE_LOG(LogTemp, Warning, TEXT("❌ 아군 기지 파괴 감지: 패배 판정"));
        }
        else
        {
            // 디버깅용
            UE_LOG(LogTemp, Error, TEXT("⚠️ 기지에 Faction 태그가 없습니다! 이름: %s"), *GetName());
        }

        // 3. 판정 결과 전달
        GM->EndStage(bIsVictory);
    }
}