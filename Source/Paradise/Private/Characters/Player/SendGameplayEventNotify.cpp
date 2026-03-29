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

		if (ACharacterBase* Character = Cast<ACharacterBase>(OwnerActor))
		{
			Character->SetCurrentMuzzleSocketInfo(SocketName, SocketTarget);

			if (EventTag.IsValid())
			{
				FGameplayEventData Payload;
				Payload.Instigator = OwnerActor;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
			}

			// 🌟 2. 새로 추가한 코드: 공격 사령탑을 호출하여 타격 판정이나 투사체를 발생시킵니다.
			Character->ExecuteAttackFromNotify(SocketName, SocketTarget, false);
		}
	}
}
