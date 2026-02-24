// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/ParadiseSquadFormationWidget.h"
#include "UI/Widgets/Squad/ParadiseSquadSlot.h"

#pragma region 생명주기
void UParadiseSquadFormationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 슬롯 초기화 및 클릭 이벤트 바인딩 람다 함수
	auto InitAndBindSlot = [&](UParadiseSquadSlot* InSlot, int32 Index)
		{
			if (InSlot)
			{
				InSlot->InitSlot(Index);
				if (!InSlot->OnSlotClicked.IsAlreadyBound(this, &UParadiseSquadFormationWidget::HandleSlotClick))
				{
					InSlot->OnSlotClicked.AddDynamic(this, &UParadiseSquadFormationWidget::HandleSlotClick);
				}
			}
		};

	// 0~2: 캐릭터 슬롯
	InitAndBindSlot(Slot_Main, 0);
	InitAndBindSlot(Slot_Sub1, 1);
	InitAndBindSlot(Slot_Sub2, 2);

	// 3~7: 유닛 슬롯
	InitAndBindSlot(Slot_Unit1, 3);
	InitAndBindSlot(Slot_Unit2, 4);
	InitAndBindSlot(Slot_Unit3, 5);
	InitAndBindSlot(Slot_Unit4, 6);
	InitAndBindSlot(Slot_Unit5, 7);
}
#pragma endregion 생명주기

#pragma region 공개 함수
void UParadiseSquadFormationWidget::UpdateSlot(int32 SlotIndex, const FSquadItemUIData& Data)
{
	UParadiseSquadSlot* TargetSlot = nullptr;

	// 인덱스 매핑 로직
	switch (SlotIndex)
	{
	case 0: TargetSlot = Slot_Main; break;
	case 1: TargetSlot = Slot_Sub1; break;
	case 2: TargetSlot = Slot_Sub2; break;
	case 3: TargetSlot = Slot_Unit1; break;
	case 4: TargetSlot = Slot_Unit2; break;
	case 5: TargetSlot = Slot_Unit3; break;
	case 6: TargetSlot = Slot_Unit4; break;
	case 7: TargetSlot = Slot_Unit5; break;
	default: break;
	}

	if (TargetSlot)
	{
		TargetSlot->UpdateSlot(Data);
	}
}

void UParadiseSquadFormationWidget::HighlightSlot(int32 SlotIndex)
{
	// 관리 대상 슬롯과 인덱스 매핑 (지역 변수로 깔끔하게 처리)
	const TMap<int32, UParadiseSquadSlot*> SlotMap = {
		{0, Slot_Main}, {1, Slot_Sub1}, {2, Slot_Sub2},
		{3, Slot_Unit1}, {4, Slot_Unit2}, {5, Slot_Unit3}, {6, Slot_Unit4}, {7, Slot_Unit5}
	};

	// 모든 슬롯을 순회하며 선택 상태 동기화
	for (const auto& Pair : SlotMap)
	{
		UParadiseSquadSlot* SlotWidget = Pair.Value;
		const int32 CurrentIndex = Pair.Key;

		if (SlotWidget)
		{
			// 요청된 인덱스와 일치하면 선택(true), 아니면 해제(false)
			const bool bShouldSelect = (CurrentIndex == SlotIndex);
			SlotWidget->SetSelected(bShouldSelect);
		}
	}
}

void UParadiseSquadFormationWidget::SetPreviewMode(bool bIsPreview)
{
	// 미리보기 모드일 경우 슬롯을 클릭하지 못하게 HitTestInvisible 처리
	// 이렇게 하면 멍청한 뷰(Dumb View)로서 시각적 기능만 수행하게 됨.
	ESlateVisibility TargetVisibility = bIsPreview ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Visible;

	Slot_Main->SetVisibility(TargetVisibility);
	Slot_Sub1->SetVisibility(TargetVisibility);
	Slot_Sub2->SetVisibility(TargetVisibility);

	Slot_Unit1->SetVisibility(TargetVisibility);
	Slot_Unit2->SetVisibility(TargetVisibility);
	Slot_Unit3->SetVisibility(TargetVisibility);
	Slot_Unit4->SetVisibility(TargetVisibility);
	Slot_Unit5->SetVisibility(TargetVisibility);
}
#pragma endregion 공개 함수

#pragma region 내부 로직
void UParadiseSquadFormationWidget::HandleSlotClick(int32 SlotIndex)
{
	// 1. 시각적 하이라이트 갱신
	HighlightSlot(SlotIndex);

	// 2. 상위(MainWidget)로 이벤트 전파
	if (OnSlotSelected.IsBound())
	{
		OnSlotSelected.Broadcast(SlotIndex);
	}
}
#pragma endregion 내부 로직