// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Summon/ParadiseSummonPanel.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/GachaSubsystem.h"
#include "Engine/DataTable.h"
#include "Components/Button.h"

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
	// 1. DataTableRowHandle에서 특정 배너의 구조체 데이터를 뽑아옵니다.
	if (CachedGI.IsValid() && !BannerDataRow.IsNull())
	{
		if (UGachaSubsystem* GachaSys = CachedGI->GetSubsystem<UGachaSubsystem>())
		{
			// "SummonPanel"은 에러 발생 시 로그에 찍힐 식별자입니다.
			if (FGachaBannerData* BannerData = BannerDataRow.GetRow<FGachaBannerData>(TEXT("SummonPanel")))
			{
				// 2. 뽑아낸 구조체 데이터를 서브시스템에 전달!
				GachaSys->InitializeBanner(*BannerData);
				UE_LOG(LogTemp, Log, TEXT("[SummonPanel] 배너 데이터 세팅 완료: %s"), *BannerDataRow.RowName.ToString());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [SummonPanel] BannerDataRow가 BP에 설정되지 않았거나 Subsystem에 접근할 수 없습니다."));
	}
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
	// 실제 컨트롤러에게 확률이 어떤지 보내야함
	if (CachedPlayerController.IsValid())
	{
		CachedPlayerController->StartGachaActionSequence(DrawCount);
		UE_LOG(LogTemp, Log, TEXT("[SummonPanel] Controller에게 %d회 소환 연출을 요청했습니다."), DrawCount);
	}
}
#pragma endregion 내부 로직