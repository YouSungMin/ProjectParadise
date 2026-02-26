// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Loading/LoadingWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

ULoadingWidget::ULoadingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 초기화 시 0%로 설정
	SetLoadingPercent(0.0f);
}

void ULoadingWidget::SetLoadingPercent(float Percent)
{
	// 1. 값 보정 (0 ~ 1)
	const float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);

	// 2. 프로그레스 바 갱신
	if (PB_LoadingBar)
	{
		PB_LoadingBar->SetPercent(ClampedPercent);
	}

	//// 3. 텍스트 갱신 (옵션)
	//if (Text_Percent)
	//{
	//	// "50%" 형태로 표시
	//	FString PercentStr = FString::Printf(TEXT("%d%%"), FMath::FloorToInt(ClampedPercent * 100.0f));
	//	Text_Percent->SetText(FText::FromString(PercentStr));
	//}

	// 4. 완료 이벤트 트리거
	if (ClampedPercent >= 1.0f)
	{
		OnLoadingComplete();
	}
}

void ULoadingWidget::SetLoadingText(FText NewText)
{
	// 추후에 로딩 팁 텍스트 바인딩이 있다면 여기서 처리
}

void ULoadingWidget::SetBackgroundImage(UTexture2D* InTexture)
{
	// 바인딩된 이미지가 있고, 전달받은 텍스처가 유효할 때만 교체
	if (Img_Background && InTexture)
	{
		Img_Background->SetBrushFromTexture(InTexture);

		// 배경은 클릭 이벤트를 막아야 하므로 HitTestInvisible 처리 (최적화)
		Img_Background->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}
