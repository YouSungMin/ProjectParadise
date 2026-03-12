// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/SendGameplayEventNotify.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/Base/CharacterBase.h"

void USendGameplayEventNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		AActor* OwnerActor = MeshComp->GetOwner();

		if (SocketName != NAME_None)
		{
			if (ACharacterBase* Character = Cast<ACharacterBase>(OwnerActor))
			{
				// 이름과 타겟(무기/몸체)을 모두 넘겨서 저장해둡니다.
				Character->SetCurrentMuzzleSocketInfo(SocketName, SocketTarget);
			}
		}

		FGameplayEventData Payload;
		Payload.Instigator = OwnerActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
	}
}
