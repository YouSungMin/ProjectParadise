// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/Stage/ParadiseStageSelectWidget.h"
#include "UI/Widgets/Lobby/Stage/ParadiseStageNodeWidget.h"
#include "UI/Widgets/Lobby/Stage/ParadiseStageDetailWidget.h"
#include "Framework/Lobby/LobbyPlayerController.h" // 컨트롤러 헤더 필수
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Components/CanvasPanel.h"
#include "Data/Structs/StageStructs.h"
#include "Framework/System/StageSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/AudioManagementSubsystem.h"
#include "Data/Assets/ParadiseFXAudioData.h"
#include "Kismet/GameplayStatics.h"


void UParadiseStageSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());

	//RefreshMapNodes();

	if (Btn_Back)
	{
		Btn_Back->OnClicked.AddDynamic(this, &UParadiseStageSelectWidget::OnClickBack);
	}

	if (UI_StageDetail)
	{
		UI_StageDetail->OnDetailClosed.AddDynamic(this, &UParadiseStageSelectWidget::HandleDetailClosed);
	}
}

void UParadiseStageSelectWidget::NativeDestruct()
{
	if (Btn_Back) Btn_Back->OnClicked.RemoveAll(this);
	CachedGI = nullptr;
	Super::NativeDestruct();
}

#pragma region 외부 인터페이스 구현
void UParadiseStageSelectWidget::InitStageMap(int32 InChapterID)
{
	//UE_LOG(LogTemp, Log, TEXT("[StageSelect] %d 챕터 진입. 해당 챕터의 노드만 활성화합니다."), InChapterID);

	CurrentChapterID = InChapterID;

	// 스위처의 인덱스를 챕터에 맞게 변경 (챕터1 -> 인덱스 0, 챕터2 -> 인덱스 1)
	if (Switcher_ChapterMaps)
	{
		int32 TargetIndex = CurrentChapterID - 1; // 1챕터면 0번 인덱스
		Switcher_ChapterMaps->SetActiveWidgetIndex(TargetIndex);
	}

	// 챕터 번호가 바뀌었으니 노드들을 다시 껐다 켭니다.
	RefreshMapNodes();
}
#pragma endregion 외부 인터페이스 구현

#pragma region 로직 구현

void UParadiseStageSelectWidget::OnClickBack()
{
	/** @section 1. 사운드 연출 */
	if (CachedGI.IsValid() && CachedGI->GlobalAudioData)
	{
		// 1. 뒤로가기 버튼 클릭 효과음 재생
		if (CachedGI->GlobalAudioData->SFX_CommonBack)
		{
			UGameplayStatics::PlaySound2D(this, CachedGI->GlobalAudioData->SFX_CommonBack);
		}

		// 2. 카메라가 다시 로비 쪽으로 슉! 빠지는 무빙 효과음 재생
		if (CachedGI->GlobalAudioData->SFX_CameraMoveSwoosh)
		{
			UGameplayStatics::PlaySound2D(this, CachedGI->GlobalAudioData->SFX_CameraMoveSwoosh);
		}
	}

	// 내 컨트롤러 찾아서 로비(None)로 돌아가달라고 요청
	if (ALobbyPlayerController* PC = GetOwningPlayer<ALobbyPlayerController>())
	{
		// "None"으로 이동하면 -> 카메라는 Main으로, UI는 로비 메뉴로 복구됨
		PC->MoveCameraToMenu(EParadiseLobbyMenu::Battle);
	}
}

void UParadiseStageSelectWidget::RefreshMapNodes()
{
	if (!Switcher_ChapterMaps || !DT_StageStats || !DT_StageAssets)
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [StageSelect] 필수 컴포넌트 또는 데이터 테이블 누락"));
		return;
	}

	// 1. 스위처에서 "현재 켜져 있는" 캔버스를 가져옵니다!
	UStageSubsystem* StageSys = CachedGI->GetSubsystem<UStageSubsystem>();
	if (!StageSys) return;

	UWidget* ActiveWidget = Switcher_ChapterMaps->GetActiveWidget();
	UCanvasPanel* ActiveCanvas = Cast<UCanvasPanel>(ActiveWidget);

	if (!ActiveCanvas) return;

	TArray<UWidget*> Children = ActiveCanvas->GetAllChildren();

	for (UWidget* Child : Children)
	{
		if (UParadiseStageNodeWidget* Node = Cast<UParadiseStageNodeWidget>(Child))
		{
			if (Node->StageID.IsNone()) continue;

			//  이미 바인딩 되어있는지 확인하고, 안 되어 있을 때만 안전하게 바인딩 (비용 절감)
			if (!Node->OnNodeClicked.IsAlreadyBound(this, &UParadiseStageSelectWidget::HandleNodeClicked))
			{
				Node->OnNodeClicked.AddDynamic(this, &UParadiseStageSelectWidget::HandleNodeClicked);
			}

			// 루프 밖에서 구해온 StageSys를 사용하여 해금 여부 확인
			if (StageSys->IsStageUnlocked(Node->StageID))
			{
				FStageStats* Stats = DT_StageStats->FindRow<FStageStats>(Node->StageID, TEXT("MapInit"));
				FStageAssets* Assets = DT_StageAssets->FindRow<FStageAssets>(Node->StageID, TEXT("MapInit"));

				if (Stats && Assets)
				{
					Node->SetupNode(*Stats, *Assets); // SetupNode 안에서 Visible 처리됨
				}
			}
			else
			{
				//  이미 숨겨진 상태면 놔두고, 보이고 있을 때만 숨깁니다. (Slate 레이아웃 갱신 방지)
				if (Node->GetVisibility() != ESlateVisibility::Collapsed)
				{
					Node->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
}

void UParadiseStageSelectWidget::HandleNodeClicked(FName SelectedStageID)
{
	// 노드가 클릭되었다는 소식을 들으면, 숨겨뒀던 상세창을 열고 데이터를 넘겨줍니다.
	if (UI_StageDetail)
	{
		UI_StageDetail->InitDetailPopup(SelectedStageID);

		if (Btn_Back) Btn_Back->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [StageSelect] UI_StageDetail이 없습니다. WBP_StageSelect 내부에 배치했는지 확인하세요."));
	}
}

void UParadiseStageSelectWidget::HandleDetailClosed()
{
	// 팝업이 닫혔다는 신호를 받으면 메인 뒤로가기 버튼을 다시 보여줍니다.
	if (Btn_Back) Btn_Back->SetVisibility(ESlateVisibility::Visible);
}

bool UParadiseStageSelectWidget::IsStageUnlocked(FName StageID)
{
	//0223 김성현 - 스테이지 서브시스템 추가에 따라 헬퍼 함수 이용토록 변경
	if (CachedGI.IsValid())
	{
		if (UStageSubsystem* StageSys = CachedGI->GetSubsystem<UStageSubsystem>())
		{
			return StageSys->IsStageUnlocked(StageID);
		}
	}
	return false;
}
#pragma endregion 로직 구현