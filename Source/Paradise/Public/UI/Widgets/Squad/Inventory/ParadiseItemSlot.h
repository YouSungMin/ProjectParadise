// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseItemSlot.generated.h"

#pragma region 전방 선언
class UImage;
class UTextBlock;
class UButton;
#pragma endregion 전방 선언

/** @brief 슬롯 클릭 시 데이터와 함께 알림 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSlotClicked, FSquadItemUIData, ItemData);

/**
 * @class UParadiseItemSlot
 * @brief 인벤토리 리스트의 개별 아이템 슬롯 UI
 * @details 아이콘, 등급 테두리, 레벨을 표시하고 클릭 이벤트를 발생시킵니다.
 */
UCLASS()
class PARADISE_API UParadiseItemSlot : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 로직
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * @brief 데이터를 받아 UI를 갱신합니다.
	 * @param InData 표시할 데이터 구조체
	 */
	UFUNCTION(BlueprintCallable, Category = "Slot")
	void UpdateSlot(const FSquadItemUIData& InData);

private:
	UFUNCTION()
	void OnButtonClicked();

	/** @brief 등급 태그에 따라 테두리 색상을 변경하는 내부 함수 */
	void UpdateRankColor(FGameplayTag RankTag);
#pragma endregion 로직

#pragma region UI 바인딩
protected:
	/** @brief 아이템 아이콘 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Icon = nullptr;

	/** @brief 등급 테두리 이미지 (RankTag에 따라 색상 변경) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_RankBorder = nullptr;

	/** @brief 레벨 표시 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Level = nullptr;

	/** @brief 수량 표시 텍스트 (무기/장비용) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Quantity = nullptr;

	/** @brief 장착 중 표시 아이콘 (선택 사항) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_EquippedMark = nullptr;

	/** @brief 클릭 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Select = nullptr;
#pragma endregion UI 바인딩

#pragma region 데이터 드리븐 설정
protected:
	/** @brief 등급 태그에 따른 테두리 색상 맵 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI", meta = (DisplayName = "등급별 색상 맵"))
	TMap<FGameplayTag, FLinearColor> RankColorMap;

	/** @brief 매칭되는 태그가 없을 때 사용할 기본 색상 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI", meta = (DisplayName = "기본 테두리 색상"))
	FLinearColor DefaultRankColor = FLinearColor::White;
#pragma endregion 데이터 드리븐 설정

#pragma region 내부 상태
private:
	/** @brief 현재 슬롯이 담고 있는 데이터 ID */
	FName CachedID = NAME_None;

	/** @brief 현재 데이터 복사본 (필요 시) */
	FSquadItemUIData CachedData;
#pragma endregion 내부 상태

#pragma region 델리게이트
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemSlotClicked OnSlotClicked;
#pragma endregion 델리게이트
};
