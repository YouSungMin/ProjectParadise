// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Summon/ParadiseSummonPanel.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/GachaSubsystem.h"
#include "Engine/DataTable.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#pragma region 생명주기
void UParadiseSummonPanel::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 이벤트 바인딩
	if (Btn_SummonSingle) Btn_SummonSingle->OnClicked.AddDynamic(this, &UParadiseSummonPanel::OnSingleSummonClicked);
	if (Btn_SummonMulti) Btn_SummonMulti->OnClicked.AddDynamic(this, &UParadiseSummonPanel::OnMultiSummonClicked);

	// 2. 컨트롤러 및 데이터 코어(GI) 캐싱 (약참조 최적화)
	CachedPlayerController = GetOwningPlayer<ALobbyPlayerController>();
	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());

	RefreshPanelData();
}

void UParadiseSummonPanel::NativeDestruct()
{
	if (Btn_SummonSingle) Btn_SummonSingle->OnClicked.RemoveAll(this);
	if (Btn_SummonMulti) Btn_SummonMulti->OnClicked.RemoveAll(this);

	CachedPlayerController = nullptr;
	CachedGI = nullptr;

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스
void UParadiseSummonPanel::RefreshPanelData()
{
	if (!CachedGI.IsValid() || BannerDataRow.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [SummonPanel] BannerDataRow 미설정 또는 GI 없음"));
		return;
	}

	UGachaSubsystem* GachaSys = CachedGI->GetSubsystem<UGachaSubsystem>();
	if (!GachaSys) return;

	FGachaBannerData* BannerData = BannerDataRow.GetRow<FGachaBannerData>(TEXT("SummonPanel"));
	if (!BannerData)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [SummonPanel] BannerData 행을 찾지 못했습니다."));
		return;
	}

	// 배너 데이터를 서브시스템에 등록 (이 시점에 CurrentBannerType이 결정됨)
	GachaSys->InitializeBanner(*BannerData);

	// 천장 UI 갱신
	RefreshPityUI();

	UE_LOG(LogTemp, Log, TEXT("[SummonPanel] 배너 초기화 완료: %s"), *BannerDataRow.RowName.ToString());
}
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
void UParadiseSummonPanel::OnSingleSummonClicked()
{
	RequestSummonAction(1);
}

void UParadiseSummonPanel::OnMultiSummonClicked()
{
	RequestSummonAction(10);
}

void UParadiseSummonPanel::RequestSummonAction(int32 DrawCount)
{
	if (!CachedPlayerController.IsValid()) return;

	CachedPlayerController->StartGachaActionSequence(DrawCount);

	// 소환 후 천장 UI 즉시 갱신
	RefreshPityUI();

	CachedGI->SaveGameData();

	UE_LOG(LogTemp, Log, TEXT("[SummonPanel] %d회 소환 요청"), DrawCount);
}

void UParadiseSummonPanel::RefreshPityUI()
{
	if (!CachedGI.IsValid()) return;

	UGachaSubsystem* GachaSys = CachedGI->GetSubsystem<UGachaSubsystem>();
	if (!GachaSys) return;

	const int32 Remaining = GachaSys->GetRemainingUntilPity();
	const int32 Current = GachaSys->GetCurrentPityStack();

	// "천장까지 N회" 텍스트
	if (Text_PityRemaining)
	{
		Text_PityRemaining->SetText(
			FText::Format(NSLOCTEXT("Summon", "PityRemaining", "천장까지 {0}회"), FText::AsNumber(Remaining)));
	}

	// "N / 50" 스택 텍스트
	if (Text_PityStack)
	{
		const int32 Threshold = Current + Remaining;
		Text_PityStack->SetText(
			FText::Format(NSLOCTEXT("Summon", "PityStack", "{0} / {1}"),
				FText::AsNumber(Current),
				FText::AsNumber(Threshold)));
	}
}
#pragma endregion 내부 로직