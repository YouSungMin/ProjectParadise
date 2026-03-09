// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Chapter/ParadiseChapterSlotWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Framework/Lobby/LobbyPlayerController.h"

#pragma region 생명주기
void UParadiseChapterSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 이벤트 바인딩
	if (Btn_Chapter)
	{
		Btn_Chapter->OnClicked.AddDynamic(this, &UParadiseChapterSlotWidget::OnChapterClicked);
	}

	// 2. 컨트롤러 1회 캐싱 (최적화)
	CachedController = GetOwningPlayer<ALobbyPlayerController>();
}

void UParadiseChapterSlotWidget::NativeDestruct()
{
	// 이벤트 안전 해제 (메모리 릭 및 댕글링 포인터 방지)
	if (Btn_Chapter)
	{
		Btn_Chapter->OnClicked.RemoveAll(this);
	}

	CachedController = nullptr;

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void UParadiseChapterSlotWidget::InitSlot(int32 InChapterID, const FText& InChapterName, bool bIsUnlocked, UTexture2D* InMapTexture)
{
	CurrentChapterID = InChapterID;
	CachedMapTexture = InMapTexture;

	// 1. 챕터 이름 텍스트 세팅
	if (Text_ChapterName)
	{
		Text_ChapterName->SetText(InChapterName);
	}

	// 2. 해금 상태에 따른 UI 처리 (버튼 비활성화 및 잠김 UI 노출)
	if (Btn_Chapter)
	{
		Btn_Chapter->SetIsEnabled(bIsUnlocked);
	}

	if (Text_LockStatus)
	{
		// 잠겨있으면 보이게, 열려있으면 숨김
		Text_LockStatus->SetVisibility(bIsUnlocked ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직 구현
void UParadiseChapterSlotWidget::OnChapterClicked()
{
	// 중앙 컨트롤 타워인 Controller에게 "이 챕터가 선택되었음"만 알립니다.
	if (CachedController.IsValid())
	{
		//컨트롤러에게 번호와 텍스처를 함께 넘겨줍니다!
		CachedController->EnterChapterMap(CurrentChapterID, CachedMapTexture);
	}
}
#pragma endregion 내부 로직 구현