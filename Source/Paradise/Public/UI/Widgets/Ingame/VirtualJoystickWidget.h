// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VirtualJoystickWidget.generated.h"

#pragma region 전방 선언
class UImage;
class AInGameController;
class UAutoCombatComponent;
#pragma endregion 전방 선언

/** 
 * @brief 조이스틱 입력 발생 시 호출되는 델리게이트 
 * @param InputVector 정규화된 입력 벡터 (X, Y 범위: -1.0 ~ 1.0)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoystickInput, FVector2D, InputVector);

/**
 * @class UVirtualJoystickWidget
 * @brief 모바일 캐릭터 이동을 위한 가상 조이스틱 위젯입니다.
 * @details 터치 및 마우스 입력을 캡처하여 시각적 피드백(Thumb 이동)과 논리적 입력(Vector Output)을 처리합니다.
 */
UCLASS()
class PARADISE_API UVirtualJoystickWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UVirtualJoystickWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

#pragma region 입력 이벤트 오버라이드
protected:
	// 터치 입력 (모바일)
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	// 마우스 입력 (PC 에디터 테스트용)
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	// [추가] 캡처가 풀렸을 때(창 밖으로 나감 등) 안전장치
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
#pragma endregion 입력 이벤트 오버라이드

#pragma region 외부 인터페이스
public:
	/** @brief 현재 조이스틱의 정규화된 입력 벡터를 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Input")
	FVector2D GetInputVector() const { return CurrentInput; }

	/** @brief 매 프레임 입력 값을 전송하는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Input")
	FOnJoystickInput OnJoystickInput;
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/** * @brief 입력 좌표를 기반으로 썸스틱의 위치와 입력 벡터를 계산합니다.
	 * @param TouchPosition 위젯 로컬 좌표계 기준의 터치/마우스 위치
	 */
	void ProcessInput(const FVector2D& TouchPosition);

	/** @brief 조이스틱을 중앙으로 초기화하고 입력을 멈춥니다. */
	void ResetJoystick();

	/**
	 * @brief 자동 전투 모드 변경 방송 수신부
	 * @details 오토 모드 켜짐(true) 시 진행 중인 이동을 취소하고 입력을 막습니다.
	 */
	UFUNCTION()
	void HandleAutoBattleStateChanged(bool bIsAuto);
#pragma endregion 내부 로직

#pragma region 위젯 바인딩
private:
	/** @brief 조이스틱 배경 (고정된 원) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Background = nullptr;

	/** @brief 움직이는 조작부 (Thumb Stick) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Thumb = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 설정 및 데이터
private:
	/** @brief 조이스틱의 반응 반경 (픽셀 단위). 배경 이미지 크기의 절반 정도로 설정하세요. */
	UPROPERTY(EditAnywhere, Category = "Paradise|Config")
	float JoystickRadius = 150.0f;

	/** @brief 터치 보정을 위한 중앙 좌표 캐싱 */
	FVector2D CenterPosition = FVector2D::ZeroVector;

	/** @brief 현재 계산된 입력 벡터 (-1 ~ 1) */
	FVector2D CurrentInput = FVector2D::ZeroVector;

	/** @brief 현재 조작 중인지 여부 (Tick 활성화 조건) */
	bool bIsInputActive = false;
#pragma endregion 설정 및 데이터
};
