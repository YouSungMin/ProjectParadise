// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/Popup/DefeatPopupWidget.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Assets/ParadiseFXAudioData.h"
#include "Kismet/GameplayStatics.h"

void UDefeatPopupWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UDefeatPopupWidget::PlayIntroAnimation()
{
	if (Anim_Intro)
	{
		PlayAnimation(Anim_Intro);
		UE_LOG(LogTemp, Log, TEXT("💀 [DefeatPopup] 패배 연출 애니메이션 큐 사인 확인! 재생 시작!"));
	}
}