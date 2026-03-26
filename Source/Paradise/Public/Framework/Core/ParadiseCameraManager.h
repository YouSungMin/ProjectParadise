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

	/** @brief 궁극기 전용 카메라 연출 시작 */
	void StartUltimateCamera(AActor* TargetActor);

	/** @brief 궁극기 전용 카메라 연출 종료 및 복귀 */
	void StopUltimateCamera();
protected:

	virtual void BeginPlay() override;



public:
	/** @brief 현재 궁극기 연출 중인지 여부 (카메라 뺏김 방지) */
	bool bIsUltimatePlaying = false;

	/** @brief  전멸 직전 마지막 시점 위치 기억용 */
	FVector LastDeathLocation = FVector::ZeroVector;

	/** @brief  전멸 직전 마지막 시점 회전 기억용 */
	FRotator LastDeathRotation = FRotator::ZeroRotator;

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

	/** @brief 카메라의 위치 (X: 앞뒤, Y: 좌우, Z: 위아래) / 기본값: 앞 250, 위 50 */
	UPROPERTY(EditAnywhere, Category = "Squad|Camera|Ultimate")
	FVector UltimateCameraOffset = FVector(500.0f, 0.0f, 100.0f);

	/** @brief 카메라가 바라볼 목표점의 오프셋 (기본값: 캐릭터의 상체(Z: 50)를 바라봄) */
	UPROPERTY(EditAnywhere, Category = "Squad|Camera|Ultimate")
	FVector UltimateLookAtOffset = FVector(0.0f, 0.0f, 50.0f);

	/** @brief 궁극기 컷신의 카메라 시야각(FOV) (기본값: 70 - 줌인 효과) */
	UPROPERTY(EditAnywhere, Category = "Squad|Camera|Ultimate")
	float UltimateCameraFOV = 70.0f;

	/** @brief 세상이 느려지는 배율 (기본값: 0.3배속) */
	UPROPERTY(EditAnywhere, Category = "Squad|Camera|Ultimate")
	float UltimateTimeDilation = 0.3f;

private:

	/** @brief 궁극기 연출을 위해 임시로 생성할 컷신 카메라 */
	UPROPERTY()
	TObjectPtr<class ACameraActor> UltimateCamera = nullptr;

	/** @brief 슬로우 모션을 해제할 때 원래 속도로 돌려주기 위해 타겟을 기억해둡니다. */
	UPROPERTY()
	TObjectPtr<AActor> CurrentUltimateTarget = nullptr;
};
