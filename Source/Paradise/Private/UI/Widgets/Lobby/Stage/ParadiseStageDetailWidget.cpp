// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/Stage/ParadiseStageDetailWidget.h"
#include "UI/Widgets/Squad/ParadiseSquadFormationWidget.h" 
#include "UI/Widgets/Lobby/Stage/ParadiseEnemyIconWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"

#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Framework/System/StageSubsystem.h"

#include "Data/Structs/StageStructs.h" 
#include "Data/Structs/UnitStructs.h"

void UParadiseStageDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());
	CachedLobbyPC = Cast<ALobbyPlayerController>(GetOwningPlayer());

	if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UParadiseStageDetailWidget::OnClickClose);
	if (Btn_Formation) Btn_Formation->OnClicked.AddDynamic(this, &UParadiseStageDetailWidget::OnClickFormation);
	if (Btn_EnterBattle) Btn_EnterBattle->OnClicked.AddDynamic(this, &UParadiseStageDetailWidget::OnClickEnterBattle);
}

void UParadiseStageDetailWidget::NativeDestruct()
{
	if (Btn_Close) Btn_Close->OnClicked.RemoveAll(this);
	if (Btn_Formation) Btn_Formation->OnClicked.RemoveAll(this);
	if (Btn_EnterBattle) Btn_EnterBattle->OnClicked.RemoveAll(this);

	CachedGI = nullptr;
	CachedLobbyPC = nullptr;

	Super::NativeDestruct();
}

void UParadiseStageDetailWidget::InitDetailPopup(FName InStageID)
{
	CachedStageID = InStageID;

	// 1. 타이틀 설정
	if (Text_StageTitle) Text_StageTitle->SetText(FText::FromName(CachedStageID));

	// 2. 내 스쿼드 UI 초기화 (기존 위젯 재활용)
	if (UI_SquadPreview)
	{
		UI_SquadPreview->SetPreviewMode(true);
	}

	// 3. 적 리스트 표기 (데이터 테이블 연동)
	SetupEnemyList(CachedStageID);

	SetVisibility(ESlateVisibility::Visible);
}

void UParadiseStageDetailWidget::SetupEnemyList(FName InStageID)
{
	if (!WrapBox_EnemyList) return;
	WrapBox_EnemyList->ClearChildren();

	if (!EnemyIconClass)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [StageDetail] EnemyIconClass가 누락되었습니다. BP 설정을 확인하세요."));
		return;
	}

	if (CachedGI.IsValid() && CachedGI->StageWaveDetailDataTable && CachedGI->EnemyAssetsDataTable)
	{
		// 1. 해당 스테이지에 등장하는 모든 몬스터 ID를 수집할 Set (중복 제거용)
		TSet<FName> UniqueEnemyIDs;

		// 2. [사용자님의 아키텍처 적용!] 
		// StageWaveDetail 테이블의 모든 행을 순회하며, TargetStageID가 현재 스테이지와 일치하는 것만 뽑아냅니다.
		TArray<FStageWaveDetail*> AllWaves;
		CachedGI->StageWaveDetailDataTable->GetAllRows<FStageWaveDetail>(TEXT("StageDetail"), AllWaves);

		for (FStageWaveDetail* WaveData : AllWaves)
		{
			// 타겟 스테이지가 현재 스테이지와 일치하고, 몬스터 ID가 존재할 경우에만 추가!
			if (WaveData && WaveData->TargetStageID == InStageID && !WaveData->MonsterID.IsNone())
			{
				UniqueEnemyIDs.Add(WaveData->MonsterID);
			}
		}

		// 3. 수집된 고유 몬스터 ID들을 기반으로 아이콘 위젯 생성 및 주입
		for (const FName& EnemyID : UniqueEnemyIDs)
		{
			if (FEnemyAssets* EnemyAsset = CachedGI->GetDataTableRow<FEnemyAssets>(CachedGI->EnemyAssetsDataTable, EnemyID))
			{
				if (UParadiseEnemyIconWidget* IconWidget = CreateWidget<UParadiseEnemyIconWidget>(this, EnemyIconClass))
				{
					// [주의] UnitStructs.h의 FEnemyAssets 구조체 안에 TSoftObjectPtr<UTexture2D> FaceIcon; 이 존재해야 합니다!
					IconWidget->SetupIcon(EnemyAsset->FaceIcon);
					WrapBox_EnemyList->AddChildToWrapBox(IconWidget);
				}
			}
		}
	}
}

void UParadiseStageDetailWidget::OnClickClose()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UParadiseStageDetailWidget::OnClickFormation()
{
	// 1. 현재 떠 있는 팝업은 가려줍니다.
	SetVisibility(ESlateVisibility::Collapsed);

	// 2. UI 상태 전환만 요청함
	if (CachedLobbyPC.IsValid())
	{
		CachedLobbyPC->SetLobbyMenu(EParadiseLobbyMenu::Squad);
	}
}

void UParadiseStageDetailWidget::OnClickEnterBattle()
{
	if (CachedStageID.IsNone()) return;

	if (UParadiseGameInstance* GI = GetGameInstance<UParadiseGameInstance>())
	{
		// 1. 서브시스템에 현재 스테이지 ID 등록
		if (UStageSubsystem* StageSys = GI->GetSubsystem<UStageSubsystem>())
		{
			StageSys->SetSelectedStageID(CachedStageID);
		}

		// 2. 맵 에셋 추출 및 로딩 시작
		if (GI->StageAssetsDataTable)
		{
			FStageAssets* Assets = GI->GetDataTableRow<FStageAssets>(GI->StageAssetsDataTable, CachedStageID);
			if (Assets && !Assets->MapAsset.IsNull())
			{
				FName LevelToOpen = FName(*Assets->MapAsset.GetAssetName());

				if (auto* LoadingSys = GI->GetSubsystem<ULevelLoadingSubsystem>())
				{
					TArray<TSoftObjectPtr<UObject>> EmptyPreloadAssets;
					LoadingSys->StartLevelTransition(LevelToOpen, NAME_None, EmptyPreloadAssets);
					return;
				}
			}
		}
	}

	UE_LOG(LogTemp, Error, TEXT("스테이지 진입 실패. 데이터가 없거나 로딩 서브시스템 누락됨."));
}
