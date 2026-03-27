// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Data/Enums/GameEnums.h"
#include "TestNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UTestNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

	// 틱마다(매 프레임) 호출됨 (공격 중인 동안 계속 검사)
public:
	/** @brief 이 노티파이가 발생할 때(즉, 궁극기 타격 순간) 궁극기 전용 카메라 및 화면 연출을 해제할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Effect")
	bool bStopUltimateEffect = true;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
protected:
	/** @brief 소켓을 어디서 찾을 것인가? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESocketTargetType SocketTarget = ESocketTargetType::CharacterBody;

	/**
	 * @brief 공격 판정 기준 소켓 이름 (예: hand_r, Muzzle_01, Jaw 등)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName SocketName = FName("hand_r");
};
