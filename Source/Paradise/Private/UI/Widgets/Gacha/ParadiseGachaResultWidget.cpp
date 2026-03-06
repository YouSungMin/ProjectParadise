// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Gacha/ParadiseGachaResultWidget.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

#pragma region 생명주기
void UParadiseGachaResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.AddDynamic(this, &UParadiseGachaResultWidget::OnConfirmClicked);
	}

	CachedPlayerController = GetOwningPlayer<ALobbyPlayerController>();
}

void UParadiseGachaResultWidget::NativeDestruct()
{
	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.RemoveAll(this);
	}

	CachedPlayerController = nullptr;

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스
void UParadiseGachaResultWidget::ShowResults(const TArray<FGachaResult>& Results)
{
	if (Results.IsEmpty()) return;

	// 1. 단일 소환(1연차)일 경우 -> 전신 일러스트 및 상세 스탯 뷰 표출
	if (Results.Num() == 1)
	{
		if (Switcher_ResultLayout)
		{
			Switcher_ResultLayout->SetActiveWidgetIndex(INDEX_SINGLE);
		}

		// 블루프린트로 단일 데이터를 넘겨줘서 텍스트/이미지를 바인딩하게 합니다.
		OnDisplaySingleResult(Results[0]);
		UE_LOG(LogTemp, Log, TEXT("[GachaResult] 1연차 상세 결과창을 표시합니다."));
	}
	// 2. 다중 소환(10연차)일 경우 -> 니케 스타일 10칸 슬롯 뷰 표출
	else
	{
		if (Switcher_ResultLayout)
		{
			Switcher_ResultLayout->SetActiveWidgetIndex(INDEX_MULTI);
		}

		// 블루프린트로 전체 배열을 넘겨줘서 ForEach문으로 슬롯 10개를 채우게 합니다.
		OnDisplayMultiResult(Results);
		UE_LOG(LogTemp, Log, TEXT("[GachaResult] 10연차 다중 슬롯 결과창을 표시합니다."));
	}
}
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
void UParadiseGachaResultWidget::OnConfirmClicked()
{
	// 1. 나 자신(결과창)을 화면에서 지운다.
	SetVisibility(ESlateVisibility::Collapsed);

	// 2. Controller에게 로비 메뉴로 복귀 요청 
	if (CachedPlayerController.IsValid())
	{
		// 카메라가 알아서 다시 Camera_Summon으로 스무스하게 이동하고 소환 팝업창이 뜹니다.
		CachedPlayerController->SetLobbyMenu(EParadiseLobbyMenu::Summon);
	}
}
#pragma endregion 내부 로직