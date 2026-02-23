// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/Stage/ParadiseStageNodeWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

#include "Kismet/GameplayStatics.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/StageStructs.h"
#include "Framework/System/LevelLoadingSubsystem.h"

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

	// 2. 이름 설정
	if (Text_StageName)
	{
		Text_StageName->SetText(InStats.StageName);
	}

	// 3. 썸네일 설정
	if (Img_Thumbnail)
	{
		UTexture2D* Tex = InAssets.Thumbnail.LoadSynchronous();
		if (Tex)
		{
			Img_Thumbnail->SetBrushFromTexture(Tex);
		}
	}

	// TODO: 잠금 여부 처리 (SaveGame 체크 로직 추가 가능)
}

void UParadiseStageNodeWidget::OnClickEnter()
{
	UE_LOG(LogTemp, Log, TEXT("[StageNode] 입장: %s"), *StageID.ToString());
	// 플레이어 컨트롤러에 게임 시작 요청

	FName LevelToOpen = NAME_None;

	// 1. 데이터 테이블에서 맵 이름 찾기
	if (auto* GI = GetGameInstance<UParadiseGameInstance>())
	{
		if (GI->StageAssetsDataTable)
		{
			FStageAssets* Assets = GI->GetDataTableRow<FStageAssets>(GI->StageAssetsDataTable, StageID);
			if (Assets && !Assets->MapAsset.IsNull())
			{
				LevelToOpen = FName(*Assets->MapAsset.GetAssetName());
			}
		}
	}
	// 2. 비상용 하드코딩
	if (LevelToOpen.IsNone())
	{
		LevelToOpen = FName("L_Stage1_1");
	}
	// 3. 서브시스템을 통한 로딩 요청
	if (UGameInstance* GI = GetGameInstance())
	{
		// 게임 인스턴스 서브시스템 가져오기
		if (auto* LoadingSys = GI->GetSubsystem<ULevelLoadingSubsystem>())
		{
			// 추가로 미리 로딩할 에셋이 없다면 빈 배열 전달
			TArray<TSoftObjectPtr<UObject>> EmptyPreloadAssets;

			// LoadingMap에 NAME_None을 넣으면 서브시스템 내부에서 "L_Loading"을 자동으로 쓰도록 설계됨
			LoadingSys->StartLevelTransition(LevelToOpen, NAME_None, EmptyPreloadAssets);

			UE_LOG(LogTemp, Log, TEXT("[StageNode] 로딩 시스템 가동! 목표: %s"), *LevelToOpen.ToString());
			return;
		}
	}
	
	// 서브시스템을 못 찾았을 때만 비상용으로 직접 이동
	UE_LOG(LogTemp, Warning, TEXT("로딩 시스템 실패, 직접 이동합니다."));
	UGameplayStatics::OpenLevel(this, LevelToOpen);
}
#pragma endregion 로직 구현