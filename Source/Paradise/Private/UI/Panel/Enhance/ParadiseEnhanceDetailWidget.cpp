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

	// 탭 타입에 따라 재화의 '단위'를 다르게 출력합니다!
	if (Text_Cost)
	{
		if (TabType == SquadTabs::Character)
		{
			// 캐릭터: 돌파 재료 (조각/영혼석 개수)
			Text_Cost->SetText(FText::FromString(FString::Printf(TEXT("%d 조각"), Cost)));
		}
		else
		{
			// 무기/방어구: 강화 비용 (골드)
			Text_Cost->SetText(FText::FromString(FString::Printf(TEXT("%d 골드"), Cost)));
		}
	}

	if (Text_CurrentStat) Text_CurrentStat->SetText(FText::FromString(CurrentStat));
	if (Text_NextStat) Text_NextStat->SetText(FText::FromString(NextStat));

	// 2. [핵심] 타입에 따른 버튼 가시성 스위칭
	if (TabType == SquadTabs::Weapon || TabType == SquadTabs::Armor)
	{
		// 장비일 경우: 강화 버튼 ON, 돌파 버튼 OFF
		if (Btn_Enhance)
		{
			Btn_Enhance->SetVisibility(ESlateVisibility::Visible);
			Btn_Enhance->SetIsEnabled(true); // 다시 활성화!
		}
		if (Btn_Breakthrough) Btn_Breakthrough->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (TabType == SquadTabs::Character)
	{
		// 캐릭터일 경우: 돌파 버튼 ON, 강화 버튼 OFF
		if (Btn_Enhance) Btn_Enhance->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_Breakthrough) Btn_Breakthrough->SetVisibility(ESlateVisibility::Visible);
	}
}

void UParadiseEnhanceDetailWidget::ClearDetail()
{
	if (Text_ItemName) Text_ItemName->SetText(FText::FromString(TEXT("대상을 선택하세요")));
	if (Text_Cost) Text_Cost->SetText(FText::FromString(TEXT("0 G")));
	if (Text_CurrentStat) Text_CurrentStat->SetText(FText::FromString(TEXT("강화 전 스탯")));
	if (Text_NextStat) Text_NextStat->SetText(FText::FromString(TEXT("강화 후 스탯")));

	// 강화 버튼을 보이게 하되, 비활성화(누를 수 없음) 처리!
	if (Btn_Enhance)
	{
		Btn_Enhance->SetVisibility(ESlateVisibility::Visible);
		Btn_Enhance->SetIsEnabled(false); // 회색으로 변하고 클릭 불가
	}
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