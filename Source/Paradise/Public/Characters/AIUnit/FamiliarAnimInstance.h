// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FamiliarAnimInstance.generated.h"

class APawn;

/**
 * 퍼밀리어 및 일반 몬스터 공용 애니메이션 인스턴스
 */
UCLASS()
class PARADISE_API UFamiliarAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 애니메이션 초기화 (블루프린트와 Initialize Animation 역할, 폰이 스폰될 때 최초실행)
	virtual void NativeInitializeAnimation() override;

	// 매 프레임 업데이트 (블루프린트의 UpdateA Animation 역할)
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/*
	 * @brief 현재 이동 속도 (BlendSpace에서 사용)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float CurrentSpeed;

	/*
	 * @brief 최적화를 위해 매번 찾지 않고 캐싱해두는 주인 폰(Pawn)
	 */
	UPROPERTY()
	TObjectPtr<APawn> OwnerPawn;
};
