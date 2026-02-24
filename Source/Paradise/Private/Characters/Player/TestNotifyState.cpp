// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/TestNotifyState.h"
#include "Characters/Base/PlayerBase.h"

void UTestNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (MeshComp)
    {
        if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
        {
            Character->ResetHitActors();
        }
    }
}

void UTestNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
	{
		Character->CheckHit(SocketName, AttackRadius);
	}
}
