// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "Data/Enums/GameEnums.h"
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

	/** @brief 소켓을 어디서 찾을 것인가? (몸체 vs 무기) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESocketTargetType SocketTarget = ESocketTargetType::CharacterBody;

	/** @brief 투사체 발사 기준 소켓 이름 (안 쓰면 None) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName SocketName = NAME_None;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
