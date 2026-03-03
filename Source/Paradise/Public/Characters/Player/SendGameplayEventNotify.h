// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "SendGameplayEventNotify.generated.h"

/**
 * @class UAnimNotify_SendGameplayEvent
 * @brief 애니메이션 특정 프레임에서 GAS로 이벤트를 쏴주는 범용 단발성 노티파이입니다.
 */
UCLASS()
class PARADISE_API USendGameplayEventNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	FGameplayTag EventTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
