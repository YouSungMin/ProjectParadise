// Copyright (C) Project Paradise. All Rights Reserved.

#include "UI/Widgets/InGame/VirtualJoystickWidget.h"

#include "Components/Image.h"
#include "Blueprint/WidgetLayoutLibrary.h"

UVirtualJoystickWidget::UVirtualJoystickWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 입력을 직접 처리하기 위해 포커스 가능 설정
	SetIsFocusable(true);
}

#pragma region 생명주기
void UVirtualJoystickWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResetJoystick();
}

void UVirtualJoystickWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 조작 중일 때만 지속적으로 입력 값을 브로드캐스트 (Pawn 이동 로직용)
	if (bIsInputActive)
	{
		OnJoystickInput.Broadcast(CurrentInput);
	}
}
#pragma endregion 생명주기

#pragma region 입력 이벤트 오버라이드
FReply UVirtualJoystickWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	bIsInputActive = true;
	CenterPosition = InGeometry.GetLocalSize() * 0.5f; // 위젯의 중앙을 기준점으로 잡음
	ProcessInput(InGeometry.AbsoluteToLocal(InGestureEvent.GetScreenSpacePosition()));

	// [핵심 수정] 터치 시작 시 위젯이 입력을 확실히 '캡처'하도록 강제합니다.
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UVirtualJoystickWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (bIsInputActive)
	{
		ProcessInput(InGeometry.AbsoluteToLocal(InGestureEvent.GetScreenSpacePosition()));
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UVirtualJoystickWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	ResetJoystick();
	// [핵심 수정] 캡처를 풀어줍니다.
	return FReply::Handled().ReleaseMouseCapture();
}

FReply UVirtualJoystickWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭 시 터치 시작과 동일하게 처리
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsInputActive = true;
		CenterPosition = InGeometry.GetLocalSize() * 0.5f;
		ProcessInput(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));

		// 마우스를 캡처하여 위젯 밖으로 나가도 드래그가 유지되도록 함
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return FReply::Unhandled();
}

FReply UVirtualJoystickWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		ResetJoystick();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply UVirtualJoystickWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsInputActive)
	{
		ProcessInput(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
void UVirtualJoystickWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
	ResetJoystick();
}
#pragma endregion 입력 이벤트 오버라이드

#pragma region 내부 로직
void UVirtualJoystickWidget::ProcessInput(const FVector2D& TouchPosition)
{
	// 1. 중심으로부터의 벡터 계산
	FVector2D Direction = TouchPosition - CenterPosition;
	const float Distance = Direction.Size();

	// 2. 벡터 길이 제한 (썸스틱이 배경 밖으로 나가지 않도록)
	FVector2D ClampedDir = Direction;
	if (Distance > JoystickRadius)
	{
		ClampedDir = Direction.GetSafeNormal() * JoystickRadius;
	}

	// 3. 정규화된 입력 값 계산 (-1.0 ~ 1.0)
	FVector2D NormalizedInput = ClampedDir / JoystickRadius;

	CurrentInput= NormalizedInput;


	// 4. Thumb 이미지 이동 (Render Translation 활용 - 성능 최적화)
	if (Img_Thumb)
	{
		Img_Thumb->SetRenderTranslation(ClampedDir);
	}
}

void UVirtualJoystickWidget::ResetJoystick()
{
	bIsInputActive = false;
	CurrentInput = FVector2D::ZeroVector;

	// 시각적 위치 초기화
	if (Img_Thumb)
	{
		Img_Thumb->SetRenderTranslation(FVector2D::ZeroVector);
	}

	// 멈춤 신호 전송 (캐릭터 정지)
	OnJoystickInput.Broadcast(FVector2D::ZeroVector);
}
#pragma endregion 내부 로직