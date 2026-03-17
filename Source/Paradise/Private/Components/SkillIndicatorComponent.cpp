// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SkillIndicatorComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
USkillIndicatorComponent::USkillIndicatorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void USkillIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		// 1. 컴포넌트 내부에서 데칼 컴포넌트를 동적으로 생성합니다.
		RangeDecal = NewObject<UDecalComponent>(Owner, TEXT("SkillRangeDecal"));
		if (RangeDecal)
		{
			// 2. 캐릭터의 Root에 부착합니다.
			RangeDecal->SetupAttachment(Owner->GetRootComponent());
			RangeDecal->RegisterComponent();

			// 3. 데칼이 바닥을 향하도록 회전시킵니다. (Pitch -90도)
			RangeDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

			// 4. 발밑으로 살짝 내려줍니다. (캐릭터의 캡슐 절반 높이 정도 내리면 좋습니다)
			RangeDecal->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));

			// 5. 마테리얼 적용 및 초기 숨김 처리
			if (IndicatorMaterial)
			{
				RangeDecal->SetDecalMaterial(IndicatorMaterial);
			}
			RangeDecal->SetVisibility(false);
		}
	}

}

void USkillIndicatorComponent::ShowIndicator(float Radius, float ForwardOffset)
{
	if (RangeDecal)
	{
		// X: 깊이, Y/Z: 반지름(Radius)
		RangeDecal->DecalSize = FVector(DecalDepth, Radius, Radius);

		// 캐릭터 앞으로 ForwardOffset만큼 밀어주고, Z축으로 바닥에 붙임(-80.0f)
		RangeDecal->SetRelativeLocation(FVector(ForwardOffset, 0.0f, -80.0f));

		RangeDecal->SetVisibility(true);

		// [로그 추가] 장판에 실제로 적용된 반지름과 오프셋 수치 출력
		UE_LOG(LogTemp, Warning, TEXT("🔵 [SkillIndicator] 장판 표시 완료! (Radius: %f / ForwardOffset: %f)"), Radius, ForwardOffset);
	}
}

void USkillIndicatorComponent::HideIndicator()
{
	if (RangeDecal)
	{
		RangeDecal->SetVisibility(false);
	}
}

