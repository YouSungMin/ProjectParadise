// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ObjectPoolInterface.h"
#include "DamageTextActor.generated.h"

#pragma region 전방 선언
class UWidgetComponent;
class UDamageTextWidget;
#pragma endregion 전방 선언


/**
 * @class ADamageTextActor
 * @brief 데미지 숫자를 3D 월드에 표시하는 풀링 가능한 액터입니다.
 * @details UWidgetComponent를 통해 3D 공간에 UMG 위젯을 표시하며,
 *          DisplayTime 이후 자동으로 풀에 반납됩니다.
 *          크리티컬 히트 여부에 따라 위젯 스타일이 달라집니다.
 */
UCLASS()
class PARADISE_API ADamageTextActor : public AActor, public IObjectPoolInterface
{
	GENERATED_BODY()
	
#pragma region 생명주기
public:
	ADamageTextActor();
#pragma endregion 생명주기

#pragma region 오브젝트 풀 인터페이스
public:
	/** @brief 풀에서 꺼내질 때 호출되어 액터를 초기화하고 활성화합니다. */
	virtual void OnPoolActivate_Implementation() override;

	/** @brief 풀로 반납될 때 호출되어 액터를 숨기고 비활성화합니다. */
	virtual void OnPoolDeactivate_Implementation() override;
#pragma endregion 오브젝트 풀 인터페이스

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 위젯에 데미지를 전달하고, 지정된 시간 후 자동으로 풀로 반납하는 타이머를 작동시킵니다.
	 * @param DamageAmount 표시할 데미지 수치
	 * @param bIsCritical 크리티컬 히트 여부 (색상/크기 변경)
	 * @param WorldLocation 3D 월드상 표시 위치 (타겟 머리 위 등)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Combat|DamageText")
	void InitializeDamageText(float DamageAmount, bool bIsCritical, const FVector& WorldLocation);
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/**
	 * @brief 수명이 다하면 스스로 오브젝트 풀에 반납합니다.
	 * @details DisplayTime 타이머가 종료되면 자동 호출됩니다.
	 */
	void ReturnSelfToPool();
#pragma endregion 내부 로직

#pragma region 데이터 드리븐 설정
protected:
	/**
	 * @brief 데미지 텍스트가 화면에 체류하는 시간 (초 단위).
	 * @details 기획자가 BP 디테일 패널에서 조절할 수 있습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageText|Config", meta = (ClampMin = "0.1", ClampMax = "5.0", DisplayName = "화면 체류 시간(초)"))
	float DisplayTime = 1.5f;

	/**
	 * @brief 위젯이 위로 떠오르는 거리 (cm 단위).
	 * @details 0이면 제자리에 고정됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageText|Config", meta = (ClampMin = "0.0", ClampMax = "500.0", DisplayName = "상승 거리(cm)"))
	float FlyUpDistance = 100.0f;

	/**
	 * @brief 위젯이 떠오르는 속도 (초당 cm).
	 * @details 높을수록 빠르게 이동합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageText|Config", meta = (ClampMin = "10.0", ClampMax = "500.0", DisplayName = "상승 속도(cm/s)"))
	float FlyUpSpeed = 100.0f;
#pragma endregion 데이터 드리븐 설정

#pragma region 컴포넌트
protected:
	/**
	 * @brief 기준점이 되는 루트 컴포넌트.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootComp = nullptr;

	/**
	 * @brief UDamageTextWidget을 3D 공간에 띄워줄 위젯 컴포넌트.
	 * @details SetWidgetSpace(Screen)으로 항상 카메라를 바라봅니다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> DamageWidgetComponent = nullptr;
#pragma endregion 컴포넌트

#pragma region 런타임 상태
private:
	/** @brief 풀 반납용 타이머 핸들 */
	FTimerHandle PoolReturnTimerHandle;

	/** @brief 애니메이션 시작 위치 (보간 계산용) */
	FVector StartLocation = FVector::ZeroVector;

	/** @brief 목표 위치 (시작 위치 + 상승 거리) */
	FVector TargetLocation = FVector::ZeroVector;
#pragma endregion 런타임 상태

};
