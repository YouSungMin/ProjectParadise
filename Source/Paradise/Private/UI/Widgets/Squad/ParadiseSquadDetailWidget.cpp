// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/ParadiseSquadDetailWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"

#pragma region 생명주기
void UParadiseSquadDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_SwapCharacter)   Btn_SwapCharacter->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleSwapChar);
	if (Btn_SwapEquipment)   Btn_SwapEquipment->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleSwapEquip);
	if (Btn_CancelEquipMode) Btn_CancelEquipMode->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleCancel);
	if (Btn_Confirm)         Btn_Confirm->OnClicked.AddDynamic(this, &UParadiseSquadDetailWidget::HandleConfirm);

	SetVisibility(ESlateVisibility::Collapsed);
}

void UParadiseSquadDetailWidget::NativeDestruct()
{
	if (Btn_SwapCharacter)   Btn_SwapCharacter->OnClicked.RemoveAll(this);
	if (Btn_SwapEquipment)   Btn_SwapEquipment->OnClicked.RemoveAll(this);
	if (Btn_CancelEquipMode) Btn_CancelEquipMode->OnClicked.RemoveAll(this);
	if (Btn_Confirm)         Btn_Confirm->OnClicked.RemoveAll(this);

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 공개 함수
void UParadiseSquadDetailWidget::ShowInfo(const FSquadItemUIData& InData, bool bIsFormationContext, bool bIsUnit)
{
	SetVisibility(ESlateVisibility::Visible);

	// 1. 이름
	if (Text_Name)
	{
		Text_Name->SetText(InData.Name.IsEmpty() ? FText::FromString(TEXT("-")) : InData.Name);
	}

	// 2. 설명/레벨
	if (Text_Desc)
	{
		if (bIsUnit)
		{
			// 유닛 슬롯이면 레벨을 무조건 숨김
			Text_Desc->SetText(FText::GetEmpty());
		}
		else
		{
			// 캐릭터 슬롯이면 레벨 표시 (레벨이 0이하라도 강제로 1로 표시하여 0번 슬롯 버그 해결)
			int32 DisplayLevel = FMath::Max(1, InData.Level);
			FString LevelText = FString::Printf(TEXT("Lv.%d"), DisplayLevel);
			Text_Desc->SetText(FText::FromString(LevelText));
		}
	}

	if (HBox_ButtonRoot)
	{
		HBox_ButtonRoot->SetVisibility(bIsFormationContext ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (!bIsFormationContext)
	{
		// 인벤토리 클릭 시 교체 버튼 강제로 숨김
		if (Btn_SwapCharacter) Btn_SwapCharacter->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_SwapEquipment) Btn_SwapEquipment->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 3. 아이콘
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
}

void UParadiseSquadDetailWidget::UpdateButtonState(ESquadUIState CurrentState, bool bIsUnitTab, bool bHasPendingSelection)
{
	bool bIsNormal = (CurrentState == ESquadUIState::Normal);

	// 인벤토리에서 선택했을 때 버튼 박스 강제로 켜주기 (ShowInfo에서 꺼졌을 수 있음)
	if (HBox_ButtonRoot && !bIsNormal)
	{
		HBox_ButtonRoot->SetVisibility(ESlateVisibility::Visible);
	}

	// 1. [상태: 교체 모드] (EquipMode, CharacterSwap 등)
	if (!bIsNormal)
	{
		// 일반 교체 버튼들은 숨김
		if (Btn_SwapCharacter)   Btn_SwapCharacter->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_SwapEquipment)   Btn_SwapEquipment->SetVisibility(ESlateVisibility::Collapsed);
		// 취소 버튼 보임
		if (Btn_CancelEquipMode) Btn_CancelEquipMode->SetVisibility(ESlateVisibility::Visible);
		// 확인 버튼 보임 및 활성화 상태 제어
		if (Btn_Confirm)
		{
			Btn_Confirm->SetVisibility(ESlateVisibility::Visible);
			// 인벤토리 아이템을 선택했으면 활성화(True), 아니면 비활성화(False -> 회색)
			Btn_Confirm->SetIsEnabled(bHasPendingSelection);
		}
	}
	// 2. [상태: 일반 모드] (Normal)
	else
	{
		// 확인/취소 버튼 숨김
		if (Btn_CancelEquipMode) Btn_CancelEquipMode->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_Confirm)         Btn_Confirm->SetVisibility(ESlateVisibility::Collapsed); // [추가]

		if (bIsUnitTab)
		{
			// 유닛은 장비 교체 불가
			if (Btn_SwapCharacter) Btn_SwapCharacter->SetVisibility(ESlateVisibility::Visible);
			if (Btn_SwapEquipment) Btn_SwapEquipment->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 캐릭터는 둘 다 가능
			if (Btn_SwapCharacter) Btn_SwapCharacter->SetVisibility(ESlateVisibility::Visible);
			if (Btn_SwapEquipment) Btn_SwapEquipment->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void UParadiseSquadDetailWidget::ClearInfo()
{
	FSquadItemUIData EmptyData;
	// 빈 정보 보여주고, 문맥 false(버튼 숨김), 유닛 아님 처리
	ShowInfo(EmptyData, false, false);

	if (Text_Name) Text_Name->SetText(FText::GetEmpty());
	if (Text_Desc) Text_Desc->SetText(FText::GetEmpty());
	if (HBox_ButtonRoot) HBox_ButtonRoot->SetVisibility(ESlateVisibility::Collapsed);
}
#pragma endregion 공개 함수

#pragma region 핸들러
void UParadiseSquadDetailWidget::HandleSwapChar()
{
	if (OnSwapCharacterClicked.IsBound()) OnSwapCharacterClicked.Broadcast();
}

void UParadiseSquadDetailWidget::HandleSwapEquip()
{
	if (OnSwapEquipmentClicked.IsBound()) OnSwapEquipmentClicked.Broadcast();
}

void UParadiseSquadDetailWidget::HandleCancel()
{
	if (OnCancelClicked.IsBound()) OnCancelClicked.Broadcast();
}

void UParadiseSquadDetailWidget::HandleConfirm()
{
	if (OnConfirmClicked.IsBound()) OnConfirmClicked.Broadcast();
}
#pragma endregion 핸들러