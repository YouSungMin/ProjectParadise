// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ParadiseCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API AParadiseCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	

public:

	/**
	 * @brief OverViewCamera를 찾아서 초기화해두는 함수
	 */
	void InitializeOverviewCamera();

	/**
	 * @brief 카메라 시점을 현재 상태(전멸 여부, 자동 모드 여부)에 따라 갱신합니다.
	 * @details SetViewTargetWithBlend를 사용하여 부드러운 시점 전환을 처리합니다.
	 */
	void UpdateCameraSystem();
protected:

	virtual void BeginPlay() override;

protected:

	/** @brief 전장을 조망하는 전체 뷰 전용 카메라 액터 (에디터에서 할당) */
	UPROPERTY(EditAnywhere, Category = "Squad|Camera")
	TObjectPtr<AActor> OverviewCameraActor = nullptr;

	/** @brief 전체 뷰 카메라를 찾기 위한 태그 (기본값: "Camera.Overview") */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Camera")
	FName OverviewCameraTag = TEXT("Camera.Overview");

	/** @brief 카메라 전환 시 걸리는 블렌딩 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Camera")
	float CameraBlendTime = 1.5f;

public:

	/** @brief  전멸 직전 마지막 시점 위치 기억용 */
	FVector LastDeathLocation = FVector::ZeroVector;

	/** @brief  전멸 직전 마지막 시점 회전 기억용 */
	FRotator LastDeathRotation = FRotator::ZeroRotator;
};
