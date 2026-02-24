// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseSquadFormationWidget.generated.h"

#pragma region 전방 선언
class UParadiseSquadSlot;
#pragma endregion 전방 선언

/** @brief 슬롯 선택 시 인덱스 전달 (0:Main, 1:Sub1...) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFormationSlotSelected, int32, SlotIndex);

/**
 * @class UParadiseSquadFormationWidget
 * @brief 좌측 상단 편성 슬롯(메인/서브/유닛)을 관리하는 뷰 위젯
 */
UCLASS()
class PARADISE_API UParadiseSquadFormationWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 공개 함수
public:
	/**
	 * @brief 지정된 인덱스를 가진 편성 슬롯의 표시 데이터를 갱신합니다.
	 * @details 메인/서브 캐릭터 혹은 유닛 슬롯의 아이콘, 레벨, 등급 정보를 업데이트합니다.
	 * @param SlotIndex 업데이트할 대상 슬롯의 고유 인덱스 (예: 0=Main, 1=Sub1, 3=Unit1).
	 * @param Data 표시에 필요한 데이터 모델 (View Model). ID가 유효하지 않으면 빈 슬롯으로 처리됩니다.
	 */
	void UpdateSlot(int32 SlotIndex, const FSquadItemUIData& Data);

	/**
	 * @brief 특정 슬롯을 시각적으로 강조(Highlight)하고, 나머지 슬롯의 강조는 해제합니다.
	 * @details 사용자의 선택을 시각적으로 피드백하기 위해 테두리 이미지를 켜거나 끕니다. (Radio Button 동작)
	 * @param SlotIndex 선택된(강조할) 슬롯의 인덱스.
	 */
	void HighlightSlot(int32 SlotIndex);

	/**
	 * @brief 위젯을 미리보기 모드로 설정합니다.
	 * @param bIsPreview true면 터치 불가(눈으로만 봄), false면 터치 가능
	 */
	void SetPreviewMode(bool bIsPreview);
#pragma endregion 공개 함수

#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
#pragma endregion 생명주기

#pragma region UI 바인딩
protected:
	// --- 캐릭터 슬롯 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UParadiseSquadSlot> Slot_Main = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UParadiseSquadSlot> Slot_Sub1 = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UParadiseSquadSlot> Slot_Sub2 = nullptr;

	// --- 유닛 슬롯 (5개) ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UParadiseSquadSlot> Slot_Unit1 = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UParadiseSquadSlot> Slot_Unit2 = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UParadiseSquadSlot> Slot_Unit3 = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UParadiseSquadSlot> Slot_Unit4 = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UParadiseSquadSlot> Slot_Unit5 = nullptr;
#pragma endregion UI 바인딩

#pragma region 내부 로직
private:
	/** @brief 슬롯 클릭 시 호출되는 핸들러 */
	UFUNCTION()
	void HandleSlotClick(int32 SlotIndex);
#pragma endregion 내부 로직

#pragma region 델리게이트
public:
	UPROPERTY(BlueprintAssignable)
	FOnFormationSlotSelected OnSlotSelected;
#pragma endregion 델리게이트
};
