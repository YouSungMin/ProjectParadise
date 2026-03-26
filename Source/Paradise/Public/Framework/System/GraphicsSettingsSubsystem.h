// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GraphicsSettingsSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UGraphicsSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * @brief 현재 그래픽 퀄리티를 반환합니다.
	 * @return 0: 낮음, 1: 보통, 2: 높음, 3: 최상
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Settings|Graphics")
	int32 GetGraphicsQuality() const;

	/**
	 * @brief 그래픽 퀄리티를 변경하고 즉시 적용 및 디스크에 저장합니다.
	 * @param NewQuality 설정할 퀄리티 단계 (0~3)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Settings|Graphics")
	void SetGraphicsQuality(int32 NewQuality);


	/**
	 * @brief 유저의 메모리를 체크하고 강제적으로 그래픽 옵션을 적용하는 함수
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Settings|Graphics")
	void CheckDevicePerformanceAndApply();
};
