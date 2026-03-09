// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Environment/ParadiseMapEnvironmentActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

// Sets default values
#pragma region 초기화 및 생명주기 구현
AParadiseMapEnvironmentActor::AParadiseMapEnvironmentActor()
{
	// [최적화] 단순히 텍스처만 갈아끼우는 배경이므로 매 프레임 연산(Tick)을 완전히 끕니다.
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트로 스태틱 메시 생성
	MapMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MapMeshComponent"));
	RootComponent = MapMeshComponent;

	// 충돌 연산이 필요 없는 단순 배경이므로 충돌을 꺼서 성능을 최적화합니다.
	MapMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AParadiseMapEnvironmentActor::BeginPlay()
{
	Super::BeginPlay();

	// [최적화] 매번 교체할 때마다 생성하지 않고, 시작할 때 다이내믹 머티리얼을 캐싱합니다.
	if (MapMeshComponent)
	{
		// 0번 인덱스의 머티리얼을 기반으로 다이내믹 머티리얼을 생성하고 적용합니다.
		DynamicMapMaterial = MapMeshComponent->CreateAndSetMaterialInstanceDynamic(0);

		if (!DynamicMapMaterial)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [MapEnvActor] 다이내믹 머티리얼 생성 실패! 메시에 머티리얼이 할당되었는지 확인하세요."));
		}
	}
}
#pragma endregion 초기화 및 생명주기 구현

#pragma region 외부 인터페이스 구현
void AParadiseMapEnvironmentActor::ChangeMapBackground(UTexture2D* NewMapTexture)
{
	if (!NewMapTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapEnvActor] 전달받은 텍스처가 유효하지 않습니다."));
		return;
	}

	// 캐싱해둔 다이내믹 머티리얼의 파라미터(TextureParameterName)에 새 텍스처를 꽂아 넣습니다.
	if (DynamicMapMaterial)
	{
		DynamicMapMaterial->SetTextureParameterValue(TextureParameterName, NewMapTexture);
		UE_LOG(LogTemp, Log, TEXT("[MapEnvActor] 지도 배경이 성공적으로 교체되었습니다: %s"), *NewMapTexture->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[MapEnvActor] 다이내믹 머티리얼이 캐싱되지 않아 텍스처를 바꿀 수 없습니다."));
	}
}
#pragma endregion 외부 인터페이스 구현

