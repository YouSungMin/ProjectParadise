// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/ParadiseSquadSlot.h"
#include "UI/Widgets/Squad/ParadiseSquadDragDrop.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

#pragma region 생명주기
void UParadiseSquadSlot::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 에디터에서 설정한 Width/Height 값을 SizeBox에 즉시 반영
	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(SlotWidth);
		RootSizeBox->SetHeightOverride(SlotHeight);
	}
}

void UParadiseSquadSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Select)
	{
		Btn_Select->OnClicked.AddDynamic(this, &UParadiseSquadSlot::OnButtonClicked);
	}

	// 초기엔 선택 안 된 상태
	if (Img_SelectionBorder)
	{
		Img_SelectionBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UParadiseSquadSlot::NativeDestruct()
{
	if (Btn_Select)
	{
		Btn_Select->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 로직 구현
void UParadiseSquadSlot::InitSlot(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	if (Text_Level)
	{
		// 3번 인덱스 이상은 유닛 슬롯 -> 레벨 표기 안 함
		if (SlotIndex >= 3)
		{
			Text_Level->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 캐릭터 슬롯은 일단 보이도록 설정 (데이터가 없으면 나중에 꺼짐)
			Text_Level->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}

void UParadiseSquadSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	// 드래그 앤 드롭을 위해 들어온 데이터를 캐싱한다
	CachedData = InData;

	// ID가 없으면 비어있는 슬롯으로 간주
	bIsEmpty = InData.ID.IsNone();

	if (bIsEmpty)
	{
		// 1. 비어있음 (Empty)
		if (Img_Icon) Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
		if (Img_EmptyPlaceholder) Img_EmptyPlaceholder->SetVisibility(ESlateVisibility::Visible);
		if (Text_Level) Text_Level->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// 2. 채워짐 (Filled)
		if (Img_Icon)
		{
			Img_Icon->SetBrushFromTexture(InData.Icon);
			Img_Icon->SetVisibility(ESlateVisibility::Visible);
		}

		if (Img_EmptyPlaceholder) Img_EmptyPlaceholder->SetVisibility(ESlateVisibility::Collapsed);

		if (Text_Level)
		{
			const bool bIsUnitSlot = (SlotIndex >= 3);
			if (bIsUnitSlot)
			{
				Text_Level->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				// 가장 안전하고 명확한 C++ 포맷팅 방식 적용 (하드코딩 및 에러 방지)
				FString LevelString = FString::Printf(TEXT("Lv.%d"), InData.Level);
				Text_Level->SetText(FText::FromString(LevelString));

				Text_Level->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}
}

void UParadiseSquadSlot::SetSelected(bool bIsSelected)
{
	if (Img_SelectionBorder)
	{
		Img_SelectionBorder->SetVisibility(bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UParadiseSquadSlot::OnButtonClicked()
{
	if (SlotIndex != -1)
	{
		OnSlotClicked.Broadcast(SlotIndex);
	}
}
#pragma endregion 로직 구현

#pragma region 드래그 앤 드롭 및 입력 제어
FReply UParadiseSquadSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	// 슬롯이 비어있지 않을 때(bIsEmpty == false)만 끌어낼 수 있도록 합니다.
	if (!bIsEmpty && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return FReply::Unhandled();
}

FReply UParadiseSquadSlot::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 드래그 없이 제자리에서 마우스를 뗐다면 클릭으로 간주!
		if (InGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			OnButtonClicked(); // 기존에 뚫어두신 슬롯 클릭 델리게이트 실행
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void UParadiseSquadSlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UParadiseSquadDragDrop* DragOperation = Cast<UParadiseSquadDragDrop>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UParadiseSquadDragDrop::StaticClass())
	);

	if (DragOperation)
	{
		// 데이터 포장 (현재 편성되어 있는 캐릭터의 데이터를 페이로드에 담음)
		DragOperation->DraggedData = CachedData;
		DragOperation->SourceSlotIndex = SlotIndex;

		// 마우스를 따라다닐 시각적 복제본 생성
		if (UParadiseSquadSlot* DragVisual = CreateWidget<UParadiseSquadSlot>(GetWorld(), GetClass()))
		{
			DragVisual->InitSlot(SlotIndex);
			DragVisual->UpdateSlot(CachedData);
			DragOperation->DefaultDragVisual = DragVisual;
		}

		//  원본 슬롯 반투명 처리 및 복구 이벤트 바인딩
		SetRenderOpacity(0.3f);
		DragOperation->OnDragCancelled.AddDynamic(this, &UParadiseSquadSlot::RestoreDragState);
		DragOperation->OnDrop.AddDynamic(this, &UParadiseSquadSlot::RestoreDragState);

		OutOperation = DragOperation;

		if (OnDragStarted.IsBound())
		{
			OnDragStarted.Broadcast(DragOperation);
		}
	}
}

bool UParadiseSquadSlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	// 우리가 만든 택배 상자가 떨어졌는지 확인
	if (UParadiseSquadDragDrop* DragOp = Cast<UParadiseSquadDragDrop>(InOperation))
	{
		// 이 슬롯(View)은 자기가 무슨 타입인지, 교체해야 하는지 판단하지 않습니다. 
		// "내 인덱스(Target)로 누군가(Source) 들어왔어!" 라고 Controller에 보고만 합니다.
		if (OnSlotDropped.IsBound())
		{
			OnSlotDropped.Broadcast(SlotIndex, DragOp->SourceSlotIndex, DragOp->DraggedData);
		}

		return true; // 드롭 이벤트를 성공적으로 처리했음을 엔진에 알림
	}

	return false;
}

void UParadiseSquadSlot::RestoreDragState(UDragDropOperation* Operation)
{
	// 드래그 종료 시 슬롯 투명도 복구
	SetRenderOpacity(1.0f);
}
#pragma endregion 드래그 앤 드롭 및 입력 제어
