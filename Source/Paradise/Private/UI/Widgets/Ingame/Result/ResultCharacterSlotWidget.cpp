// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/Result/ResultCharacterSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Data/Structs/UnitStructs.h"

void UResultCharacterSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UResultCharacterSlotWidget::SetSlotData(const FResultCharacterData& InData)
{
	// 1. 초상화 설정
	if (Img_Portrait && InData.PortraitImage)
	{
		Img_Portrait->SetBrushFromTexture(InData.PortraitImage);
		Img_Portrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 2. 이름 설정
	if (Text_Name)
	{
		Text_Name->SetText(InData.CharacterName);
		Text_Name->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 3. 경험치 텍스트 (+150 Exp)
	if (Text_GainedExp)
	{
		FString ExpString = FString::Printf(TEXT("+%d EXP"), InData.GainedExp);
		Text_GainedExp->SetText(FText::FromString(ExpString));
		Text_GainedExp->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 4. 경험치 바 (비율)
	if (ProgressBar_Exp)
	{
		const float ClampedPercent = FMath::Clamp(InData.ExpPercent, 0.0f, 1.0f);
		ProgressBar_Exp->SetPercent(InData.ExpPercent);
	}
}
