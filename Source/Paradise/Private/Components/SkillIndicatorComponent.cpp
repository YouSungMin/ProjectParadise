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
void USkillIndicatorComponent::ShowIndicator(float AttackRange, float AttackRadius, float ForwardOffset)
{
	if (RangeDecal)
	{
		// CheckHit의 기본 오프셋 보정
		float BaseOffset = 100.0f;

		// 실제 스피어 트레이스가 시작되는 중심점 위치
		float ActualTraceStart = BaseOffset + ForwardOffset;

		// 실제 데칼 절반 길이
		float ActualHalfLength = (AttackRange * 0.5f) + AttackRadius;

		// 데칼 크기 적용
		// X: 투사 깊이, Y: 좌우 두께(Radius), Z: 캡슐의 절반 길이
		RangeDecal->DecalSize = FVector(DecalDepth, AttackRadius, ActualHalfLength);

		// 데칼 위치 적용 (캡슐의 정중앙)
		float CenterX = ActualTraceStart + (AttackRange * 0.5f);

		RangeDecal->SetRelativeLocation(FVector(CenterX, 0.0f, -80.0f));
		RangeDecal->SetVisibility(true);

		// [로그 추가] 수치 확인용
		UE_LOG(LogTemp, Warning, TEXT("🔵 [SkillIndicator] 완벽 보정 적용! (실제 시작점: %f, 총길이: %f)"), ActualTraceStart - AttackRadius, ActualHalfLength * 2.0f);
	}
}

void USkillIndicatorComponent::HideIndicator()
{
	if (RangeDecal)
	{
		RangeDecal->SetVisibility(false);
	}
}

