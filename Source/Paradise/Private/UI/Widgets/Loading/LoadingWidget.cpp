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

void ULoadingWidget::InitLoadingImage(FName CurrentLevel, FName TargetLevel, TSoftObjectPtr<UTexture2D> InDefaultStageImage)
{
	TSoftObjectPtr<UTexture2D> FinalImage = nullptr;

	// 1. 문자열을 소문자로 변환하여 대소문자 이슈 방지 (최적화)
	FString CurrentMapName = CurrentLevel.ToString().ToLower();
	FString TargetMapName = TargetLevel.ToString().ToLower();

	// 2. 상황 판단 우선순위 (Data-Driven Priority)
	// 🚨 [해결 포인트] Title 키워드가 포함되어 있고, 목적지가 Lobby(로비)인 경우
	if (CurrentMapName.Contains(TEXT("title")) && TargetMapName.Contains(TEXT("lobby")))
	{
		FinalImage = SpecialImages.TitleToLobby;
		UE_LOG(LogTemp, Log, TEXT("[LoadingUI] Situation: Title -> Lobby"));
	}
	// 🚨 목적지가 로비인데, 출발지가 타이틀이 아니었다면 (스테이지 -> 로비)
	else if (TargetMapName.Contains(TEXT("lobby")))
	{
		FinalImage = SpecialImages.StageToLobby;
		UE_LOG(LogTemp, Log, TEXT("[LoadingUI] Situation: Stage -> Lobby"));
	}
	// 그 외 일반 스테이지 진입 상황
	else if (!InDefaultStageImage.IsNull())
	{
		FinalImage = InDefaultStageImage;
		UE_LOG(LogTemp, Log, TEXT("[LoadingUI] Situation: Enter Stage"));
	}

	// 3. 최종 폴백 (Fallback)
	if (FinalImage.IsNull())
	{
		FinalImage = SpecialImages.DefaultBackground;
	}

	// 4. 결정된 이미지 적용
	if (!FinalImage.IsNull())
	{
		SetBackgroundImage(FinalImage.LoadSynchronous());
	}
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
	// 3. 완료 이벤트 트리거
	if (ClampedPercent >= 1.0f)
	{
		OnLoadingComplete();
	}
}

void ULoadingWidget::SetLoadingText(FText InName, FText InDesc)
{
	// 1. 스테이지 이름 텍스트 갱신
	if (Text_StageName)
	{
		// 데이터가 없으면 위젯을 숨기고, 있으면 텍스트 설정 후 노출
		if (InName.IsEmpty())
		{
			Text_StageName->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Text_StageName->SetText(InName);
			Text_StageName->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	// 2. 스테이지 설명/팁 텍스트 갱신
	if (Text_StageDesc)
	{
		if (InDesc.IsEmpty())
		{
			Text_StageDesc->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Text_StageDesc->SetText(InDesc);
			Text_StageDesc->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
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
