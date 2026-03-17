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

void USkillIndicatorComponent::ShowIndicator(float Range)
{
	if (RangeDecal)
	{
		// 데칼의 크기를 설정합니다. 
		// X축은 투사 깊이(Depth), Y와 Z축이 원의 반지름(Radius) 역할을 합니다.
		RangeDecal->DecalSize = FVector(DecalDepth, Range, Range);
		RangeDecal->SetVisibility(true);
	}
}

void USkillIndicatorComponent::HideIndicator()
{
	if (RangeDecal)
	{
		RangeDecal->SetVisibility(false);
	}
}

