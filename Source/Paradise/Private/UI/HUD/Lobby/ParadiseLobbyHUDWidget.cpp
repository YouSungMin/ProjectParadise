// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Lobby/ParadiseLobbyHUDWidget.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Components/WidgetSwitcher.h"

#include "UI/Widgets/Lobby/ParadiseLobbyTopBarWidget.h"
#include "UI/Panel/Lobby/ParadiseLobbyMenuPanelWidget.h"
#include "UI/Widgets/Squad/ParadiseSquadMainWidget.h"

void UParadiseLobbyHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 컨트롤러에 HUD 등록
	if (ALobbyPlayerController* PC = GetOwningPlayer<ALobbyPlayerController>())
	{
		PC->SetLobbyHUD(this);
	}

	// 초기화 시 None(메인 로비) 상태로 시작
	UpdateMenuStats(EParadiseLobbyMenu::None);
}

void UParadiseLobbyHUDWidget::UpdateMenuStats(EParadiseLobbyMenu InCurrentMenu)
{
	// 방어 코드: 필수 위젯 누락 시 중단
	if (!Switcher_Content || !WBP_MenuPanel || !WBP_TopBar) return;

	// -------------------------------------------------------------------------
	// 1. UI 상태 결정 (State Determination)
	// -------------------------------------------------------------------------
	const bool bIsMainLobby = (InCurrentMenu == EParadiseLobbyMenu::None);

	// TopBar를 숨겨야 하는 메뉴인지 확인 (Squad 포함 필수)
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
		// 1. 강제 가시성 확보 (내부적으로 Collapsed 되었을 가능성 배제)
		TargetWidget->SetVisibility(ESlateVisibility::Visible);

		// 2. 편성(Squad) 위젯 특수 처리: 델리게이트 재연결 및 데이터 갱신
		if (UParadiseSquadMainWidget* SquadWidget = Cast<UParadiseSquadMainWidget>(TargetWidget))
		{
			// 델리게이트 바인딩 확인 (중복 바인딩 방지)
			if (!SquadWidget->OnBackRequested.IsAlreadyBound(this, &UParadiseLobbyHUDWidget::HandleSquadBackRequest))
			{
				SquadWidget->OnBackRequested.AddDynamic(this, &UParadiseLobbyHUDWidget::HandleSquadBackRequest);
			}

			// ★ [핵심] 캐시된 위젯은 NativeConstruct가 다시 호출되지 않으므로, 수동으로 갱신 함수를 호출해야 함.
			// 이 호출이 없으면 두 번째 진입 시 빈 화면이거나 갱신되지 않은 상태가 됨.
			SquadWidget->RefreshInventoryUI();
		}

		// 3. 스위처의 활성 위젯 변경 (화면 전환)
		Switcher_Content->SetActiveWidget(TargetWidget);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[HUD] 위젯 생성 실패! 메뉴: %d"), (int32)InCurrentMenu);
	}
}

void UParadiseLobbyHUDWidget::HandleSquadBackRequest()
{
    // 편성 화면에서 뒤로가기를 누르면 메인 로비(None)로 상태를 변경합니다.
	if (ALobbyPlayerController* PC = GetOwningPlayer<ALobbyPlayerController>())
	{
		// 컨트롤러의 CurrentMenu 변수를 'None(0)'으로 갱신시킵니다.
		// 컨트롤러가 내부적으로 다시 HUD->UpdateMenuStats(None)을 호출해주므로 화면도 정상적으로 바뀝니다.
		PC->SetLobbyMenu(EParadiseLobbyMenu::None);
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
