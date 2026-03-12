// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "UI/Widgets/Squad/ParadiseSquadDragDrop.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/Button.h"

void UParadiseItemSlot::NativeConstruct()
{
	Super::NativeConstruct();
	//if (Btn_Select)
	//{
	//	Btn_Select->OnClicked.AddDynamic(this, &UParadiseItemSlot::OnButtonClicked);
	//}
}

void UParadiseItemSlot::NativeDestruct()
{
	/*if (Btn_Select)
	{
		Btn_Select->OnClicked.RemoveAll(this);
	}*/
	Super::NativeDestruct();
}

void UParadiseItemSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	CachedData = InData;

	// 1. 공통 아이콘 설정
	if (Img_Icon)
	{
		if (InData.Icon)
		{
			Img_Icon->SetBrushFromTexture(InData.Icon);
			Img_Icon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Img_Icon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 2. 공통 등급 테두리 갱신
	UpdateRankColor(InData.Rarity);

	// 3. 공통 장착 마크 갱신
	if (Img_EquippedMark)
	{
		Img_EquippedMark->SetVisibility(InData.bIsEquipped ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 보유 여부에 따른 상태 변경 (도감용)
	if (!InData.bIsOwned)
	{
		// 아이콘을 흑백/어둡게 (블러 느낌)
		if (Img_Icon) Img_Icon->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 0.9f));
		if (Img_RankBorder) Img_RankBorder->SetColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f));

		// 터치(클릭) 원천 차단
		if (Btn_Select) Btn_Select->SetIsEnabled(false);
	}
	else
	{
		// 보유한 아이템이면 정상 색상 및 클릭 활성화
		if (Img_Icon) Img_Icon->SetColorAndOpacity(FLinearColor::White);
		if (Img_RankBorder) Img_RankBorder->SetColorAndOpacity(FLinearColor::White); // 틴트 초기화
		if (Btn_Select) Btn_Select->SetIsEnabled(true);
	}
}

void UParadiseItemSlot::OnButtonClicked()
{
	if (OnSlotClicked.IsBound())
	{
		OnSlotClicked.Broadcast(CachedData);
	}
}

void UParadiseItemSlot::UpdateRankColor(EItemRarity Rarity)
{
	if (!Img_RankBorder) return;

	// Map에서 Enum 값으로 색상을 찾습니다.
	FLinearColor* FoundColor = RankColorMap.Find(Rarity);
	Img_RankBorder->SetColorAndOpacity(FoundColor ? *FoundColor : DefaultRankColor);
}

#pragma region 드래그 앤 드롭 로직
FReply UParadiseItemSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	// 좌클릭(모바일 터치) 시 드래그 판정 시작
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return FReply::Unhandled();
}

FReply UParadiseItemSlot::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 드래그를 하지 않고 손을 뗐으며, 마우스가 위젯 영역 안에 있고, 아이템을 보유 중이라면 클릭으로 판정!
		if (CachedData.bIsOwned && InGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			OnButtonClicked();
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void UParadiseItemSlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	// 최적화: 미보유 상태인 아이템은 드래그할 수 없도록 원천 차단
	if (!CachedData.bIsOwned) return;

	// 우리가 만든 택배 상자(Payload) 생성
	UParadiseSquadDragDrop* DragOperation = Cast<UParadiseSquadDragDrop>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UParadiseSquadDragDrop::StaticClass())
	);

	if (DragOperation)
	{
		// 데이터 포장
		DragOperation->DraggedData = CachedData;
		DragOperation->SourceSlotIndex = -1; // -1: 인벤토리에서 출발했음을 의미

		// 현재 내 클래스(GetClass())와 동일한 위젯을 새로 하나 만들어서 똑같은 데이터를 넣어줍니다.
		if (UParadiseItemSlot* DragVisual = CreateWidget<UParadiseItemSlot>(GetWorld(), GetClass()))
		{
			DragVisual->UpdateSlot(CachedData);
			DragOperation->DefaultDragVisual = DragVisual;
		}

		// 원본 슬롯 시각적 피드백 (반투명 처리하여 비워진 느낌 주기)
		SetRenderOpacity(0.3f);

		// 드래그가 끝나면(드롭 성공이든 허공에 취소든) 다시 선명하게 복구하도록 이벤트 구독
		DragOperation->OnDragCancelled.AddDynamic(this, &UParadiseItemSlot::RestoreDragState);
		DragOperation->OnDrop.AddDynamic(this, &UParadiseItemSlot::RestoreDragState);

		OutOperation = DragOperation;

		// 컨트롤러(MainWidget)에게 "나 드래그 시작했어! 다른 슬롯들 회색으로 만들어!" 라고 신호 전파
		if (OnDragStarted.IsBound())
		{
			OnDragStarted.Broadcast(DragOperation);
		}
	}
}

void UParadiseItemSlot::RestoreDragState(UDragDropOperation* Operation)
{
	// 드래그가 완전히 종료되면 다시 원상 복구
	SetRenderOpacity(1.0f);
}
#pragma endregion 드래그 앤 드롭 로직