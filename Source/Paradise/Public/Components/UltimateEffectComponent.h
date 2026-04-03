// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UltimateEffectComponent.generated.h"

#pragma region 전방 선언
class APostProcessVolume;
#pragma endregion 전방 선언

/**
 * @class UUltimateEffectComponent
 * @brief 궁극기 사용 시 화면 전체에 포스트 프로세스(원신/명조 스타일)를 부드럽게 적용하는 컴포넌트입니다.
 * @details AInGameController에 부착하여 캐릭터 태그(교체)와 무관하게 전역적으로 연출을 제어합니다. (SRP 준수)
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARADISE_API UUltimateEffectComponent : public UActorComponent
{
	GENERATED_BODY()

#pragma region 초기화 및 생명주기
public:
	UUltimateEffectComponent();
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#pragma endregion 초기화 및 생명주기

#pragma region 외부 인터페이스 (Controller -> Component)
public:
	/**
	 * @brief 궁극기 전용 포스트 프로세스 연출을 부드럽게 시작합니다.
	 * @param Duration 연출이 유지되는 총 시간 (초 단위)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Effect")
	void PlayUltimateEffect(float Duration = 2.0f);

	/** @brief 연출을 강제로 즉시 종료합니다. */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Effect")
	void StopUltimateEffect();
#pragma endregion 외부 인터페이스 (Controller -> Component)

//#pragma region 내부 컴포넌트 및 에셋
//private:
//	/** @brief 전역 화면 효과를 담당할 포스트 프로세스 컴포넌트 (동적 생성) */
//	UPROPERTY(VisibleAnywhere, Category = "Paradise|Components")
//	TObjectPtr<UPostProcessComponent> PostProcessComp = nullptr;
//#pragma endregion 내부 컴포넌트 및 에셋

#pragma region 데이터 드리븐 설정
private:
	/** @brief 페이드 인(서서히 화면이 변함)에 걸리는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Effect|Config")
	float FadeInTime = 0.2f;

	/** @brief 페이드 아웃(서서히 원래 화면으로 복귀)에 걸리는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Effect|Config")
	float FadeOutTime = 0.3f;
#pragma endregion 데이터 드리븐 설정

#pragma region 내부 상태 데이터
private:
	/** @brief 레벨에 배치된 궁극기 전용 PostProcessVolume 약참조 */
	UPROPERTY()
	TWeakObjectPtr<APostProcessVolume> CachedPPVolume = nullptr;

	bool  bIsPlaying = false;
	float CurrentTime = 0.0f;
	float TotalDuration = 2.0f;
#pragma endregion 내부 상태 데이터
};
