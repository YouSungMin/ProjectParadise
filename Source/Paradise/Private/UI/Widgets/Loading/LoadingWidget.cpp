// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Loading/LoadingWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Animation/WidgetAnimation.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

ULoadingWidget::ULoadingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 초기화 시 0%로 설정
	SetLoadingPercent(0.0f);
	bIsDisappearing = false;

	// 2. 리테이너 박스 가시성 보장
	if (Retainer_TransitionEffect)
	{
		Retainer_TransitionEffect->SetVisibility(ESlateVisibility::Visible);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULevelLoadingSubsystem* LoadingSys = GI->GetSubsystem<ULevelLoadingSubsystem>())
		{
			if (LoadingSys->IsAppearingPhase())
			{
				// ✅ 한 프레임 뒤에 재생 → InitLoadingImage 세팅 후 재생 보장
				if (GetWorld())
				{
					GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
						{
							if (Anim_Appear)
							{
								PlayAnimation(Anim_Appear);
							}
						});
				}
			}
		}
	}
}
void ULoadingWidget::InitAsCovered()
{
	// MPC Progress를 1로 즉시 세팅하여 화면 완전히 가림
	if (MPC_Loading) // 에디터에서 할당
	{
		UKismetMaterialLibrary::SetScalarParameterValue(
			GetWorld(), MPC_Loading, FName("Progress"), 1.3f);
	}
}
void ULoadingWidget::PlayDisappearAnim()
{
	if (Anim_Disappear && !bIsDisappearing)
	{
		bIsDisappearing = true;
		PlayAnimation(Anim_Disappear);
	}
}
void ULoadingWidget::ShowContent()
{
	if (MPC_Loading)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(
			GetWorld(), MPC_Loading, FName("Progress"), 0.0f);
	}
}
void ULoadingWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULevelLoadingSubsystem* LoadingSys = GI->GetSubsystem<ULevelLoadingSubsystem>())
		{
			if (Animation == Anim_Appear)
			{
				// ✅ Anim_Appear 완료 → 서브시스템에 알림 → OpenLevel(L_Loading)
				LoadingSys->NotifyAppearFinished();
			}
			else if (Animation == Anim_Disappear)
			{
				// ✅ Anim_Disappear 완료 → 서브시스템에 알림 → 위젯 제거
				LoadingSys->NotifyDisappearFinished();
				OnLoadingComplete();
			}
		}
	}
}

void ULoadingWidget::InitLoadingImage(FName CurrentLevel, FName TargetLevel, TSoftObjectPtr<UTexture2D> InDefaultStageImage)
{
	TSoftObjectPtr<UTexture2D> FinalImage = nullptr;

	// 1. 문자열을 소문자로 변환하여 대소문자 이슈 방지 (최적화)
	FString CurrentMapName = CurrentLevel.ToString().ToLower();
	FString TargetMapName = TargetLevel.ToString().ToLower();

	// 2. 상황 판단 우선순위 (Data-Driven Priority)
	// Title 키워드가 포함되어 있고, 목적지가 Lobby(로비)인 경우
	if (CurrentMapName.Contains(TEXT("title")) && TargetMapName.Contains(TEXT("lobby")))
	{
		FinalImage = SpecialImages.TitleToLobby;
		//UE_LOG(LogTemp, Log, TEXT("[LoadingUI] Situation: Title -> Lobby"));
	}
	// 목적지가 로비인데, 출발지가 타이틀이 아니었다면 (스테이지 -> 로비)
	else if (TargetMapName.Contains(TEXT("lobby")))
	{
		FinalImage = SpecialImages.StageToLobby;
		//UE_LOG(LogTemp, Log, TEXT("[LoadingUI] Situation: Stage -> Lobby"));
	}
	// 그 외 일반 스테이지 진입 상황
	else if (!InDefaultStageImage.IsNull())
	{
		FinalImage = InDefaultStageImage;
		//UE_LOG(LogTemp, Log, TEXT("[LoadingUI] Situation: Enter Stage"));
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
