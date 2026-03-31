// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Lobby/ParadiseLobbyHUDWidget.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/AudioManagementSubsystem.h"
#include "Framework/System/ParadiseCursorSubsystem.h"
#include "Components/WidgetSwitcher.h"

#include "UI/Widgets/Squad/ParadiseSquadMainWidget.h"
#include "UI/Widgets/Lobby/ParadiseLobbyTopBarWidget.h"
#include "UI/Panel/Lobby/ParadiseLobbyMenuPanelWidget.h"
#include "UI/Widgets/Enhance/ParadiseEnhancePopupWidget.h"
#include "UI/Widgets/Codex/ParadiseCodexMainWidget.h"

#include "Data/Assets/ParadiseFXAudioData.h"

void UParadiseLobbyHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 생성될 때 딱 한 번만 무거운 탐색 연산을 수행하여 캐싱합니다!
	CachedController = GetOwningPlayer<ALobbyPlayerController>();

	if (CachedController.IsValid())
	{
		CachedController->SetLobbyHUD(this);
	}

	// 게임 인스턴스 캐싱 (최적화)
	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());

	/** @section 로비 BGM 재생 요청 */
	if (CachedGI.IsValid())
	{
		if (UAudioManagementSubsystem* AudioMag = CachedGI->GetSubsystem<UAudioManagementSubsystem>())
		{
			// GameInstance가 미리 들고 있는 전역 오디오 데이터셋에서 로비 BGM을 꺼냅니다 (Data-Driven)
			if (CachedGI->GlobalAudioData && CachedGI->GlobalAudioData->BGM_Lobby)
			{
				// 이전 음악(타이틀 등)은 자동으로 페이드아웃되고 로비 음악이 재생됩니다.
				AudioMag->PlayBGM(CachedGI->GlobalAudioData->BGM_Lobby);
			}
		}
	}

	// 초기화 시 None(메인 로비) 상태로 시작
	UpdateMenuStats(EParadiseLobbyMenu::None);
}

FReply UParadiseLobbyHUDWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UParadiseCursorSubsystem* CursorSys = GI->GetSubsystem<UParadiseCursorSubsystem>())
		{
			CursorSys->UpdateCursorPosition(InMouseEvent.GetScreenSpacePosition());
			CursorSys->ShowCursor(true);
		}
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UParadiseLobbyHUDWidget::UpdateMenuStats(EParadiseLobbyMenu InCurrentMenu)
{
	// 방어 코드: 필수 위젯 누락 시 중단
	if (!Switcher_Content || !WBP_MenuPanel || !WBP_TopBar) return;

	// -------------------------------------------------------------------------
	// 1. UI 상태 결정 (State Determination)
	// -------------------------------------------------------------------------
	const bool bIsMainLobby = (InCurrentMenu == EParadiseLobbyMenu::None);

	if (CachedGI.IsValid())
	{
		if (UAudioManagementSubsystem* AudioMag = CachedGI->GetSubsystem<UAudioManagementSubsystem>())
		{
			// 메인 로비로 복귀 시 로비 BGM 재생
			if (bIsMainLobby)
			{
				if (CachedGI->GlobalAudioData && CachedGI->GlobalAudioData->BGM_Lobby)
				{
					AudioMag->PlayBGM(CachedGI->GlobalAudioData->BGM_Lobby);
				}
			}
		}
	}

	// StageSelect 위젯은 카메라 이동 후 LobbyPlayerController가 뷰포트에 직접 띄웁니다.
	if (InCurrentMenu == EParadiseLobbyMenu::StageMap)
	{
		WBP_TopBar->SetVisibility(ESlateVisibility::Collapsed);
		WBP_MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
		Switcher_Content->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const bool bHideTopBar = (
		InCurrentMenu == EParadiseLobbyMenu::Battle ||
		InCurrentMenu == EParadiseLobbyMenu::Summon ||
		InCurrentMenu == EParadiseLobbyMenu::Squad ||
		InCurrentMenu == EParadiseLobbyMenu::Enhance ||
		InCurrentMenu == EParadiseLobbyMenu::Codex);

	// -------------------------------------------------------------------------
	// 2. 가시성 적용 (Visibility Application)
	// -------------------------------------------------------------------------

	// 메인 로비 UI (메뉴 패널) 처리
	WBP_MenuPanel->SetVisibility(bIsMainLobby ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// 콘텐츠 스위처 처리
	Switcher_Content->SetVisibility(bIsMainLobby ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	// [수정] 상단 바 처리 (숨김 조건에 해당하면 Collapsed)
	WBP_TopBar->SetVisibility(bHideTopBar ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	// 메인 로비라면 여기서 종료 (최적화)
	if (bIsMainLobby) return;

	// -------------------------------------------------------------------------
	// 3. 위젯 관리 (Create or Reuse)
	// -------------------------------------------------------------------------

	UUserWidget* TargetWidget = nullptr;

	// [A] 캐시 검색
	if (TObjectPtr<UUserWidget>* CachedWidgetPtr = CreatedMenuWidgets.Find(InCurrentMenu))
	{
		TargetWidget = *CachedWidgetPtr;

		// 캐시는 되어있으나 Switcher에서 제거된 경우(RemoveFromParent 등) 대비하여 복구
		if (TargetWidget && !Switcher_Content->HasChild(TargetWidget))
		{
			Switcher_Content->AddChild(TargetWidget);
		}
	}

	// [B] 캐시 없음 -> 신규 생성
	if (!TargetWidget)
	{
		if (TSubclassOf<UUserWidget>* ClassPtr = MenuWidgetClasses.Find(InCurrentMenu))
		{
			if (ClassPtr && *ClassPtr)
			{
				TargetWidget = CreateWidget<UUserWidget>(this, *ClassPtr);
				if (TargetWidget)
				{
					Switcher_Content->AddChild(TargetWidget);
					CreatedMenuWidgets.Add(InCurrentMenu, TargetWidget);
				}
			}
		}
	}

	// -------------------------------------------------------------------------
	// 4. 위젯 활성화 및 데이터 갱신 (Refresh & Activate)
	// -------------------------------------------------------------------------

	if (TargetWidget)
	{
		TargetWidget->SetVisibility(ESlateVisibility::Visible);

		// [편성창 특수 처리]
		if (UParadiseSquadMainWidget* SquadWidget = Cast<UParadiseSquadMainWidget>(TargetWidget))
		{
			// [최적화] 통합된 핸들러(HandleBackToMainLobby)로 연결
			if (!SquadWidget->OnBackRequested.IsAlreadyBound(this, &UParadiseLobbyHUDWidget::HandleBackToMainLobby))
			{
				SquadWidget->OnBackRequested.AddDynamic(this, &UParadiseLobbyHUDWidget::HandleBackToMainLobby);
			}
			SquadWidget->RefreshInventoryUI();
		}
		// [강화창 특수 처리]
		else if (UParadiseEnhancePopupWidget* EnhanceWidget = Cast<UParadiseEnhancePopupWidget>(TargetWidget))
		{
			// [최적화] 통합된 핸들러(HandleBackToMainLobby)로 연결
			if (!EnhanceWidget->OnBackRequested.IsAlreadyBound(this, &UParadiseLobbyHUDWidget::HandleBackToMainLobby))
			{
				EnhanceWidget->OnBackRequested.AddDynamic(this, &UParadiseLobbyHUDWidget::HandleBackToMainLobby);
			}
			EnhanceWidget->RefreshInventory();
		}
		// [도감창 특수 처리]
		else if (UParadiseCodexMainWidget* CodexWidget = Cast<UParadiseCodexMainWidget>(TargetWidget))
		{
			if (!CodexWidget->OnBackRequested.IsAlreadyBound(this, &UParadiseLobbyHUDWidget::HandleBackToMainLobby))
			{
				CodexWidget->OnBackRequested.AddDynamic(this, &UParadiseLobbyHUDWidget::HandleBackToMainLobby);
			}
		}
		Switcher_Content->SetActiveWidget(TargetWidget);
	}
	if (InCurrentMenu == EParadiseLobbyMenu::Squad)
	{
		// 1. TMap에서 이미 생성/캐싱된 Squad 위젯 포인터를 찾습니다.
		if (auto* FoundWidget = CreatedMenuWidgets.Find(EParadiseLobbyMenu::Squad))
		{
			// 2. 찾아낸 UserWidget을 SquadMainWidget으로 캐스팅합니다.
			if (UParadiseSquadMainWidget* SquadWidget = Cast<UParadiseSquadMainWidget>(*FoundWidget))
			{
				// 3. 카메라 전환 및 3D 모델링 갱신 함수 실행!
				SquadWidget->OnEnterSquadUI();
			}
		}
	}
}

void UParadiseLobbyHUDWidget::OnStartCameraMove()
{
	// 상단바, 메뉴 패널, 콘텐츠 스위처 모두 숨김
	if (WBP_TopBar) WBP_TopBar->SetVisibility(ESlateVisibility::Collapsed);
	if (WBP_MenuPanel) WBP_MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (Switcher_Content) Switcher_Content->SetVisibility(ESlateVisibility::Collapsed);

	// 팁: 애니메이션(Fade Out)을 재생하면 더 고급스럽습니다.
}

void UParadiseLobbyHUDWidget::HandleBackToMainLobby()
{
	if (CachedController.IsValid())
	{
		CachedController->RequestBackToPreviousMenu();
	}

	// 로비로 돌아갈 때(팝업이 닫힐 때) 게임 데이터 자동 저장!
	if (CachedGI.IsValid())
	{
		CachedGI->SaveGameData();
	}
}
