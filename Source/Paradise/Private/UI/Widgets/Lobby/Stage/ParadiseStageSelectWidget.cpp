// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/Stage/ParadiseStageSelectWidget.h"
#include "UI/Widgets/Lobby/Stage/ParadiseStageNodeWidget.h"
#include "Framework/Lobby/LobbyPlayerController.h" // 컨트롤러 헤더 필수
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Data/Structs/StageStructs.h"

void UParadiseStageSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshMapNodes();

	if (Btn_Back)
	{
		Btn_Back->OnClicked.AddDynamic(this, &UParadiseStageSelectWidget::OnClickBack);
	}
}

#pragma region 로직 구현

void UParadiseStageSelectWidget::OnClickBack()
{
	// 내 컨트롤러 찾아서 로비(None)로 돌아가달라고 요청
	if (ALobbyPlayerController* PC = GetOwningPlayer<ALobbyPlayerController>())
	{
		// "None"으로 이동하면 -> 카메라는 Main으로, UI는 로비 메뉴로 복구됨
		PC->MoveCameraToMenu(EParadiseLobbyMenu::None);
	}
}

void UParadiseStageSelectWidget::RefreshMapNodes()
{
	if (!Canvas_MapArea || !DT_StageStats || !DT_StageAssets)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [StageSelect] 필수 컴포넌트 또는 데이터 테이블 누락"));
		return;
	}

	// 1. 캔버스 내의 모든 자식 위젯을 가져옵니다.
	TArray<UWidget*> Children = Canvas_MapArea->GetAllChildren();

	for (UWidget* Child : Children)
	{
		// 2. StageNodeWidget인지 확인
		if (UParadiseStageNodeWidget* Node = Cast<UParadiseStageNodeWidget>(Child))
		{
			// ID가 없으면 무시
			if (Node->StageID.IsNone()) continue;

			// 3. 해금 여부 확인 (핵심: 해금 안 됐으면 Collapsed 상태 유지)
			if (IsStageUnlocked(Node->StageID))
			{
				// 4. 데이터 조회 및 주입
				FStageStats* Stats = DT_StageStats->FindRow<FStageStats>(Node->StageID, TEXT("MapInit"));
				FStageAssets* Assets = DT_StageAssets->FindRow<FStageAssets>(Node->StageID, TEXT("MapInit"));

				if (Stats && Assets)
				{
					// 데이터 주입과 동시에 Visibility를 Visible로 변경
					Node->SetupNode(*Stats, *Assets);
				}
			}
			else
			{
				// 해금 안 된 스테이지는 숨김 (스포일러 방지)
				Node->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

bool UParadiseStageSelectWidget::IsStageUnlocked(FName StageID)
{
	// TODO: 실제로는 GameInstance의 SaveGame에서 'MaxClearedStage' 등을 가져와 비교해야 함.
	// 예시: "1-1"은 항상 해금, 나머지는 세이브 데이터 확인.

	if (StageID == FName("1-1")) return true; // 1-1은 무조건 보임

	// 테스트용: 1-1 클리어 가정, 1-2까지 보임 (실제 개발 시엔 세이브 데이터와 연동하세요)
	// if (StageID == FName("1-2")) return true; 

	return false;
}
#pragma endregion 로직 구현