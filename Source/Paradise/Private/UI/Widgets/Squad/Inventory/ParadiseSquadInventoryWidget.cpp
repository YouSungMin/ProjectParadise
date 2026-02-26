// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/ParadiseSquadInventoryWidget.h"
#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "UI/Widgets/Squad/Inventory/Slots/ParadiseCharacterSlot.h"
#include "UI/Widgets/Squad/Inventory/Slots/ParadiseEquipmentSlot.h"
#include "UI/Widgets/Squad/Inventory/Slots/ParadiseUnitSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WrapBox.h"

#pragma region 공개 함수
void UParadiseSquadInventoryWidget::UpdateList(int32 TabIndex, const TArray<FSquadItemUIData>& ListData)
{
	if (!Switcher_List) return;

	// 1. 탭 전환
	Switcher_List->SetActiveWidgetIndex(TabIndex);

	// 2. 현재 탭에 맞는 WrapBox 찾기
	UWrapBox* TargetWrap = nullptr;
	TSubclassOf<UParadiseItemSlot> TargetSlotClass = nullptr;

	switch (TabIndex)
	{
	case SquadTabs::Character:
		TargetWrap = Wrap_Character;
		TargetSlotClass = CharacterSlotClass;
		break;
	case SquadTabs::Weapon:
		TargetWrap = Wrap_Weapon;
		TargetSlotClass = EquipmentSlotClass;
		break;
	case SquadTabs::Armor:
		TargetWrap = Wrap_Armor;
		TargetSlotClass = EquipmentSlotClass; // 장비 슬롯 공유
		break;
	case SquadTabs::Unit:
		TargetWrap = Wrap_Unit;
		TargetSlotClass = UnitSlotClass;
		break;
	}

	// 3. 방어 코드: WrapBox나 슬롯 클래스가 할당되지 않았으면 즉시 중단
	if (!TargetWrap || !TargetSlotClass)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [InventoryWidget] WrapBox 또는 슬롯 클래스가 누락되었습니다. BP 설정을 확인하세요."));
		return;
	}

	// 4. 기존 슬롯 제거 (초기화)
	TargetWrap->ClearChildren();

	// 5. 데이터 기반 슬롯 생성 (Loop 최적화)
	for (const auto& Data : ListData)
	{
		// 부모 클래스 포인터 타입으로 위젯 생성
		if (UParadiseItemSlot* NewSlot = CreateWidget<UParadiseItemSlot>(this, TargetSlotClass))
		{
			// (1) 가상 함수 호출: C++이 런타임에 어떤 자식인지 스스로 판단하여 알맞은 UpdateSlot을 실행함.
			NewSlot->UpdateSlot(Data);

			// (2) 클릭 이벤트 바인딩 (부모에 구현된 델리게이트를 바로 사용)
			NewSlot->OnSlotClicked.AddDynamic(this, &UParadiseSquadInventoryWidget::HandleSlotClick);

			// (3) 화면에 추가
			TargetWrap->AddChildToWrapBox(NewSlot);
		}
	}
}
#pragma endregion 공개 함수

#pragma region 내부 로직
void UParadiseSquadInventoryWidget::HandleSlotClick(FSquadItemUIData ItemData)
{
	// 클릭된 데이터를 상위로 전파
	if (OnItemClicked.IsBound())
	{
		OnItemClicked.Broadcast(ItemData);
	}
}
#pragma endregion 내부 로직