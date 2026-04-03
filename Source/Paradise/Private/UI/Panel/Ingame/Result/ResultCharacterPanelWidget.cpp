// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/Result/ResultCharacterPanelWidget.h"
#include "UI/Widgets/Ingame/Result/ResultCharacterSlotWidget.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"

void UResultCharacterPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

#pragma region 로직 구현
void UResultCharacterPanelWidget::UpdateCharacterSlots(const TArray<FResultCharacterData>& InCharacterDataList)
{
	if (!Container_Slots)
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [ResultPanel] Container_Slots가 바인딩되지 않았습니다!"));
		return;
	}

	if (!SlotWidgetClass)
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [ResultPanel] SlotWidgetClass가 설정되지 않았습니다!"));
		return;
	}

	// 1. 기존 슬롯 제거 (간단한 구현을 위해 Reset. 최적화하려면 Object Pool 사용 가능)
	Container_Slots->ClearChildren();

	// 2. 데이터 개수만큼 슬롯 생성
	for (const FResultCharacterData& Data : InCharacterDataList)
	{
		UResultCharacterSlotWidget* NewSlot = CreateWidget<UResultCharacterSlotWidget>(this, SlotWidgetClass);
		if (NewSlot)
		{
			// 데이터 주입
			NewSlot->SetSlotData(Data);

			// (1) 컨테이너에 추가하고, 슬롯 객체 리턴
			UPanelSlot* PanelSlot = Container_Slots->AddChild(NewSlot);

			// (2) HorizontalBoxSlot으로 형변환하여 "에디터 설정값" 적용
			if (UHorizontalBoxSlot* HorizSlot = Cast<UHorizontalBoxSlot>(PanelSlot))
			{
				// 하드코딩 제거 -> 변수(UPROPERTY) 사용
				HorizSlot->SetPadding(SlotPadding);
				HorizSlot->SetHorizontalAlignment(SlotHorizontalAlignment);
				HorizSlot->SetVerticalAlignment(SlotVerticalAlignment);

				// 사이즈 정책 적용
				HorizSlot->SetSize(FSlateChildSize(SlotSizeRule));
			}
		}
	}

	//UE_LOG(LogTemp, Log, TEXT("[ResultPanel] %d개의 캐릭터 슬롯 갱신 완료."), InCharacterDataList.Num());
}
#pragma endregion 로직 구현