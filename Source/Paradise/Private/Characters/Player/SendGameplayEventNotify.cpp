// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/SendGameplayEventNotify.h"
#include "AbilitySystemBlueprintLibrary.h"

void USendGameplayEventNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		AActor* OwnerActor = MeshComp->GetOwner();

		FGameplayEventData Payload;
		Payload.Instigator = OwnerActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
	}
}
