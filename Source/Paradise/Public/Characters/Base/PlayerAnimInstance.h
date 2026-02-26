// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Data/Enums/GameEnums.h"
#include "PlayerAnimInstance.generated.h"

class ACharacterBase;

/**
 * 
 */
UCLASS()
class PARADISE_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 애니메이션 초기화 (블루프린트와 Initialize Animation 역할, 폰이 스폰될 때 최초실행)
	virtual void NativeInitializeAnimation() override;

	// 매 프레임 업데이트 (블루프린트의 UpdateA Animation 역할)
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<APawn> OwnerPawn;
	/*
	 * @brief 현재 이동 속도 (BlendSpace에서 사용)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float CurrentSpeed;

	/*
	 * 캐릭터가 현재 들고 있는 무기 타입 (검, 활 등 무기에 따른 애니메이션 분기용)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EWeaponType CurrentWeaponType;
};
