// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/TestNotifyState.h"
#include "Characters/Base/CharacterBase.h"
#include "GAS/Attributes/BaseAttributeSet.h"

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
		FCombatActionData CurrentData = Character->GetCurrentActionData();

		// 안전 장치 
		float FinalRange = (CurrentData.AttackRange > 0.0f) ? CurrentData.AttackRange : AttackRadius; // 기존 노티파이의 AttackRadius 변수를 임시로 길이에 매핑
		float FinalRadius = (CurrentData.AttackRadius > 0.0f) ? CurrentData.AttackRadius : 40.0f; // 두께 기본값 40
		float FinalOffset = CurrentData.ForwardOffset;

		Character->CheckHit(SocketName, FinalRange, FinalRadius, FinalOffset);
	}
}
