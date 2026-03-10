// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/UltimateEffectComponent.h"
#include "GameFramework/PlayerController.h"  
#include "Camera/PlayerCameraManager.h"

#include "Engine/PostProcessVolume.h"
#include "Kismet/GameplayStatics.h"

#pragma region 초기화 및 생명주기 구현
UUltimateEffectComponent::UUltimateEffectComponent()
{
	// [최적화] 부드러운 페이드 효과를 위해 Tick을 쓰지만, 평소에는 무조건 꺼둡니다!
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UUltimateEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	// 레벨에서 "UltimatePostProcess" 태그가 달린 PostProcessVolume 을 1회 탐색 후 캐싱
	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APostProcessVolume::StaticClass(), FoundVolumes);

	for (AActor* Actor : FoundVolumes)
	{
		if (Actor->ActorHasTag(FName("UltimatePostProcess")))
		{
			CachedPPVolume = Cast<APostProcessVolume>(Actor);
			break;
		}
	}

	if (CachedPPVolume.IsValid())
	{
		// 초기엔 완전히 꺼둠
		CachedPPVolume->BlendWeight = 0.0f;
		UE_LOG(LogTemp, Log, TEXT("✅ [UltimateEffect] PostProcessVolume 캐싱 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("❌ [UltimateEffect] 태그 'UltimatePostProcess' 가 달린 PostProcessVolume 이 없습니다!"
				" 레벨에 배치하고 태그를 추가하세요."));
	}

}

void UUltimateEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsPlaying || !CachedPPVolume.IsValid()) return;

	CurrentTime += DeltaTime;
	float TargetWeight = 1.0f;

	// 페이드 인 (0 → 1)
	if (CurrentTime < FadeInTime)
	{
		TargetWeight = FMath::Clamp(CurrentTime / FadeInTime, 0.0f, 1.0f);
	}
	// 페이드 아웃 (1 → 0)
	else if (CurrentTime > TotalDuration - FadeOutTime)
	{
		const float FadeOutStart = TotalDuration - FadeOutTime;
		TargetWeight = 1.0f - FMath::Clamp(
			(CurrentTime - FadeOutStart) / FadeOutTime, 0.0f, 1.0f);
	}

	// ★ Volume 의 BlendWeight 만 조절 — 나머지 설정은 에디터에서 담당
	CachedPPVolume->BlendWeight = TargetWeight;

	if (CurrentTime >= TotalDuration)
	{
		StopUltimateEffect();
	}
}
#pragma endregion 초기화 및 생명주기 구현

#pragma region 외부 인터페이스 구현
void UUltimateEffectComponent::PlayUltimateEffect(float Duration)
{
	if (!CachedPPVolume.IsValid())
	{
		UE_LOG(LogTemp, Error,
			TEXT("❌ [UltimateEffect] CachedPPVolume 이 없습니다! 레벨 배치 및 태그를 확인하세요."));
		return;
	}

	TotalDuration = FMath::Max(Duration, FadeInTime + FadeOutTime + 0.1f);
	CurrentTime = 0.0f;
	bIsPlaying = true;
	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Log,
		TEXT("🌟 [UltimateEffect] 궁극기 화면 연출 시작! (지속: %.1f초)"), TotalDuration);
}

void UUltimateEffectComponent::StopUltimateEffect()
{
	bIsPlaying = false;

	if (CachedPPVolume.IsValid())
	{
		CachedPPVolume->BlendWeight = 0.0f;
	}

	SetComponentTickEnabled(false);
	UE_LOG(LogTemp, Log, TEXT("🌟 [UltimateEffect] 궁극기 화면 연출 종료."));
}
#pragma endregion 외부 인터페이스 구현

