// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Chapter/ParadiseChapterSlotWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#pragma region 생명주기
void UParadiseChapterSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 이벤트 바인딩
	if (Btn_Chapter)
	{
		Btn_Chapter->OnClicked.AddDynamic(this, &UParadiseChapterSlotWidget::OnChapterClicked);
	}

}

void UParadiseChapterSlotWidget::NativeDestruct()
{
	// 이벤트 안전 해제 (메모리 릭 및 댕글링 포인터 방지)
	if (Btn_Chapter)
	{
		Btn_Chapter->OnClicked.RemoveAll(this);
	}

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
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직 구현
void UParadiseChapterSlotWidget::OnChapterClicked()
{
	if (OnChapterSelected.IsBound())
	{
		OnChapterSelected.Broadcast(CurrentChapterID, CachedMapTexture);
	}
}
#pragma endregion 내부 로직 구현