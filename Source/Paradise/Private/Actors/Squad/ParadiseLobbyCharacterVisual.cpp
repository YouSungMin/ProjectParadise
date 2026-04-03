// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Squad/ParadiseLobbyCharacterVisual.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"

// Sets default values
AParadiseLobbyCharacterVisual::AParadiseLobbyCharacterVisual()
{
	PrimaryActorTick.bCanEverTick = false; // 시각적 용도이므로 틱 비활성화 (최적화)

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	// 로비용이므로 콜리전 완벽 차단
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

#pragma region 외부 인터페이스
void AParadiseLobbyCharacterVisual::SetVisual(USkeletalMesh* InMesh, UAnimSequence* InAnim)
{
	if (!MeshComponent) return;

	// 데이터가 없으면 슬롯이 비어있는 것이므로 숨김 처리
	if (!InMesh)
	{
		MeshComponent->SetVisibility(false);
		return;
	}

	// 1. 메시 교체 및 표시
	MeshComponent->SetSkeletalMeshAsset(InMesh); // UE 5.1 이상 권장 API
	MeshComponent->SetVisibility(true);

	// 2. 애니메이션 재생 (루핑)
	if (InAnim)
	{
		// 애니메이션 블루프린트 없이 단일 시퀀스 바로 재생 (최적화)
		MeshComponent->PlayAnimation(InAnim, true);
	}
}
#pragma endregion 외부 인터페이스

