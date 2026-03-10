// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/Stage/ParadiseStageNodeWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

#include "Kismet/GameplayStatics.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/StageStructs.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Framework/System/StageSubsystem.h"

void UParadiseStageNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Enter)
	{
		Btn_Enter->OnClicked.AddDynamic(this, &UParadiseStageNodeWidget::OnClickEnter);
	}
}

#pragma region 로직 구현
void UParadiseStageNodeWidget::SetupNode(const FStageStats& InStats, const FStageAssets& InAssets)
{
	// 1. 데이터가 들어왔다는 건 해금되었다는 뜻이므로 보이게 설정
	SetVisibility(ESlateVisibility::Visible);

	// 2. 썸네일 설정
	if (Img_Thumbnail)
	{
		UTexture2D* LoadedThumbnail = nullptr;

		// 3-1. 이미 메모리에 로드되어 있는지 검사 (O(1) 접근으로 스파이크 방지)
		if (InAssets.Thumbnail.IsValid())
		{
			LoadedThumbnail = InAssets.Thumbnail.Get();
		}
		// 3-2. 로드되진 않았지만 경로 데이터가 있다면 그때서야 동기 로드 수행
		else if (!InAssets.Thumbnail.IsNull())
		{
			LoadedThumbnail = InAssets.Thumbnail.LoadSynchronous();
		}

		// 3-3. 최종적으로 텍스처 적용 (없으면 비워줌)
		if (LoadedThumbnail)
		{
			Img_Thumbnail->SetBrushFromTexture(LoadedThumbnail);
		}
		else
		{
			Img_Thumbnail->SetBrushFromTexture(nullptr);
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [StageNode] %s 스테이지의 썸네일 에셋이 없습니다."), *InStats.StageName.ToString());
		}
	}

	// TODO: 잠금 여부 처리 (SaveGame 체크 로직 추가 가능)
}

void UParadiseStageNodeWidget::OnClickEnter()
{
	if (StageID.IsNone()) return;

	UE_LOG(LogTemp, Log, TEXT("[StageNode] 노드 클릭됨. 팝업 호출 요청. ID: %s"), *StageID.ToString());

	if (OnNodeClicked.IsBound())
	{
		OnNodeClicked.Broadcast(StageID);
	}
}
#pragma endregion 로직 구현