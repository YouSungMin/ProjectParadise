// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/AIUnit/FamiliarAnimInstance.h"
#include "GameFramework/Pawn.h"

void UFamiliarAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 스폰될 때 딱 한번 기억(캐싱), 핵심 최적화
	OwnerPawn = TryGetPawnOwner();
}

void UFamiliarAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	//주인이 없거나 죽어서 사라짐 방지, IsValid 체크
	if (OwnerPawn == nullptr)
	{
		return;
	}

	// GetVelocity -> VectorLength -> SetCurrentSpeed
	// X와 Y축(바닥 평면)의 이동 길이만 구합니다.
	// Size()가 아닌 Size2D()를 쓰는 이유: 공중에 떴거나 떨어질 때(Z축 속도) 달리는 애니메이션이 나오는 것을 막기 위함!
	CurrentSpeed = OwnerPawn->GetVelocity().Size2D();
}
