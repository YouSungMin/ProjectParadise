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

	//UE_LOG(LogTemp, Log, TEXT("[StageNode] 입장: %s"), *StageID.ToString());
	//// 플레이어 컨트롤러에 게임 시작 요청

	//FName LevelToOpen = NAME_None;

	//// 1. 데이터 테이블에서 맵 이름 찾기
	//if (auto* GI = GetGameInstance<UParadiseGameInstance>())
	//{
	//	//0223 - 김성현 스테이지 ID 연결 초기화 플로우 연결
	//	if (UStageSubsystem* StageSys = GI->GetSubsystem<UStageSubsystem>())
	//	{
	//		StageSys->SetSelectedStageID(StageID);

	//		// 🌟 [디버그 1] 저장된 ID를 화면과 로그에 출력
	//		FString DebugMsg = FString::Printf(TEXT("👉 [로비] 선택한 스테이지 전달 중... ID: %s"), *StageID.ToString());
	//		UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugMsg);
	//		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, DebugMsg);
	//	}

	//	if (GI->StageAssetsDataTable)
	//	{
	//		FStageAssets* Assets = GI->GetDataTableRow<FStageAssets>(GI->StageAssetsDataTable, StageID);
	//		if (Assets && !Assets->MapAsset.IsNull())
	//		{
	//			LevelToOpen = FName(*Assets->MapAsset.GetAssetName());
	//		}
	//	}
	//}
	//// 2. 비상용 하드코딩
	//if (LevelToOpen.IsNone())
	//{
	//	LevelToOpen = FName("L_Stage1_1");
	//}
	//// 3. 서브시스템을 통한 로딩 요청
	//if (UGameInstance* GI = GetGameInstance())
	//{
	//	// 게임 인스턴스 서브시스템 가져오기
	//	if (auto* LoadingSys = GI->GetSubsystem<ULevelLoadingSubsystem>())
	//	{
	//		// 추가로 미리 로딩할 에셋이 없다면 빈 배열 전달
	//		TArray<TSoftObjectPtr<UObject>> EmptyPreloadAssets;

	//		// LoadingMap에 NAME_None을 넣으면 서브시스템 내부에서 "L_Loading"을 자동으로 쓰도록 설계됨
	//		LoadingSys->StartLevelTransition(LevelToOpen, NAME_None, EmptyPreloadAssets);

	//		UE_LOG(LogTemp, Log, TEXT("[StageNode] 로딩 시스템 가동! 목표: %s"), *LevelToOpen.ToString());
	//		return;
	//	}
	//}
	//
	//// 서브시스템을 못 찾았을 때만 비상용으로 직접 이동
	//UE_LOG(LogTemp, Warning, TEXT("로딩 시스템 실패, 직접 이동합니다."));
	//UGameplayStatics::OpenLevel(this, LevelToOpen);
}
#pragma endregion 로직 구현