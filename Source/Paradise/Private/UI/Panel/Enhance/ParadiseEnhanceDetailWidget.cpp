// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Enhance/ParadiseEnhanceDetailWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

#pragma region 생명주기 구현
void UParadiseEnhanceDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Enhance) Btn_Enhance->OnClicked.AddDynamic(this, &UParadiseEnhanceDetailWidget::HandleEnhanceBtn);
	if (Btn_Breakthrough) Btn_Breakthrough->OnClicked.AddDynamic(this, &UParadiseEnhanceDetailWidget::HandleBreakthroughBtn);

	ClearDetail(); // 초기화
}

void UParadiseEnhanceDetailWidget::NativeDestruct()
{
	if (Btn_Enhance) Btn_Enhance->OnClicked.RemoveAll(this);
	if (Btn_Breakthrough) Btn_Breakthrough->OnClicked.RemoveAll(this);

	Super::NativeDestruct();
}
#pragma endregion 생명주기 구현

#pragma region 렌더링 로직
void UParadiseEnhanceDetailWidget::RefreshDetail(const FSquadItemUIData& ItemData, int32 TabType, int32 Cost, const FString& CurrentStat, const FString& NextStat)
{
	// 1. 공통 정보 세팅
	if (Img_ItemIcon && ItemData.Icon) Img_ItemIcon->SetBrushFromTexture(ItemData.Icon);
	if (Text_ItemName) Text_ItemName->SetText(ItemData.Name);
	if (Text_Cost) Text_Cost->SetText(FText::FromString(FString::Printf(TEXT("%d G"), Cost)));
	if (Text_CurrentStat) Text_CurrentStat->SetText(FText::FromString(CurrentStat));
	if (Text_NextStat) Text_NextStat->SetText(FText::FromString(NextStat));

	// 2. [핵심] 타입에 따른 버튼 가시성 스위칭
	if (TabType == SquadTabs::Weapon || TabType == SquadTabs::Armor)
	{
		// 장비일 경우: 강화 버튼 ON, 돌파 버튼 OFF
		if (Btn_Enhance) Btn_Enhance->SetVisibility(ESlateVisibility::Visible);
		if (Btn_Breakthrough) Btn_Breakthrough->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (TabType == SquadTabs::Character)
	{
		// 캐릭터일 경우: 돌파 버튼 ON, 강화 버튼 OFF
		if (Btn_Enhance) Btn_Enhance->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_Breakthrough) Btn_Breakthrough->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		// 유닛 등 지원하지 않는 탭일 경우 모두 숨김
		if (Btn_Enhance) Btn_Enhance->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_Breakthrough) Btn_Breakthrough->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UParadiseEnhanceDetailWidget::ClearDetail()
{
	if (Text_ItemName) Text_ItemName->SetText(FText::FromString(TEXT("대상을 선택하세요")));
	if (Text_Cost) Text_Cost->SetText(FText::FromString(TEXT("0 G")));
	if (Text_CurrentStat) Text_CurrentStat->SetText(FText::FromString(TEXT("-")));
	if (Text_NextStat) Text_NextStat->SetText(FText::FromString(TEXT("-")));

	if (Btn_Enhance) Btn_Enhance->SetVisibility(ESlateVisibility::Collapsed);
	if (Btn_Breakthrough) Btn_Breakthrough->SetVisibility(ESlateVisibility::Collapsed);
}
void UParadiseEnhanceDetailWidget::PlayEnhancementFX(bool bSuccess)
{
	if (bSuccess)
	{
		if (Sound_EnhanceSuccess)
		{
			UGameplayStatics::PlaySound2D(this, Sound_EnhanceSuccess);
		}
		// 파티클 재생 로직 필요시 추가
	}
	else
	{
		if (Sound_EnhanceFail)
		{
			UGameplayStatics::PlaySound2D(this, Sound_EnhanceFail);
		}
	}
}
#pragma endregion 렌더링 로직

#pragma region 이벤트 브로드캐스트
void UParadiseEnhanceDetailWidget::HandleEnhanceBtn() { OnEnhanceClicked.Broadcast(); }
void UParadiseEnhanceDetailWidget::HandleBreakthroughBtn() { OnBreakthroughClicked.Broadcast(); }
#pragma endregion 이벤트 브로드캐스트