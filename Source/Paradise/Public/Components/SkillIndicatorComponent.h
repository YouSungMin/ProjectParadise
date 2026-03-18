// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillIndicatorComponent.generated.h"

class UDecalComponent;
class UMaterialInterface;


/**
 * @brief 스킬 또는 공격의 사거리를 시각적으로 표시해주는 전용 컴포넌트입니다.
 * @note 플레이어 캐릭터, 보스 몬스터, 함정 등 사거리 장판(Decal) 표시가 필요한 모든 액터에 부착하여 완벽하게 재사용할 수 있습니다.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARADISE_API USkillIndicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillIndicatorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	

	/**
	 * @brief 사거리 장판을 화면에 표시하고 원하는 크기로 설정합니다.
	 * @param Range 표시할 장판의 반지름(사거리) 값입니다.
	 * @note UI에서 스킬 버튼을 누르고 있을 때(Hold) 또는 AI가 공격을 준비(Cast)할 때 호출하세요.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Indicator")
	void ShowIndicator(float AttackRange, float AttackRadius, float ForwardOffset);

	/**
	 * @brief 켜져 있는 사거리 장판을 즉시 숨깁니다.
	 * @note UI에서 스킬 버튼을 뗄 때(Release) 또는 스킬이 발동되거나 취소되는 시점에 호출하세요.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Indicator")
	void HideIndicator();

protected:

	/**
	 * @brief 런타임에 동적으로 생성되어 오너 액터의 발밑에 부착되는 데칼 컴포넌트 캐시입니다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> RangeDecal = nullptr;

	/**
	 * @brief 장판에 적용될 마테리얼입니다. (예: 원형 링, 부채꼴 테두리 등)
	 * @note 블루프린트 디테일 패널에서 반드시 Translucent 및 Deferred Decal 도메인으로 설정된 마테리얼을 할당해야 합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Indicator")
	TObjectPtr<UMaterialInterface> IndicatorMaterial =nullptr;

	/**
	 * @brief 데칼이 바닥으로 투사되는 깊이(Z축 방향 투사 길이)입니다.
	 * @note 값이 너무 작으면 계단이나 굴곡진 지형에서 장판이 잘려 보일 수 있습니다. 기본값 200.0f 정도면 일반적인 지형을 잘 덮어줍니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Indicator")
	float DecalDepth = 200.0f;
		
};
