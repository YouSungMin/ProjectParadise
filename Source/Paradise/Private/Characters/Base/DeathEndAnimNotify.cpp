// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Base/DeathEndAnimNotify.h"
#include "Characters/Base/CharacterBase.h"

void UDeathEndAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		// 액터를 CharacterBase로 캐스팅하여 사망 종료 함수 호출
		if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
		{
			Character->OnDeathAnimationFinished();
		}
	}
}
