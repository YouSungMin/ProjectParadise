// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Base/PlayerAnimInstance.h"
#include "Characters/Base/CharacterBase.h"

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	//스폰될 때 폰 가져와서 기억해두기
	OwnerPawn = TryGetPawnOwner();

	if (ACharacterBase* CharBase = Cast<ACharacterBase>(TryGetPawnOwner()))
	{
		//CurrentWeaponType = CharBase->GetCurrentWeaponType();
	}
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 주인이 없거나 파괴되었을 경우(안전장치), 아래 로직을 돌리지 않고 빠져나갑니다.
	if (OwnerPawn == nullptr)
	{
		return;
	}

	// 매 프레임마다 Z축(낙하)을 제외한 순수 바닥 평면(X, Y) 이동 속도만 계산합니다.
	CurrentSpeed = OwnerPawn->GetVelocity().Size2D();
}
