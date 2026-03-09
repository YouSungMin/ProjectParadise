// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Structs/GachaTypes.h"
#include "Data/Enums/GameEnums.h"
#include "ParadiseGachaResultSlotWidget.generated.h"

#pragma region 전방 선언
class UImage;
class UTextBlock;
class UBorder;
class UTexture2D;
#pragma endregion 전방 선언

/**
 * @class UParadiseGachaResultSlotWidget
 * @brief 10연차 결과창의 슬롯 하나를 담당하는 위젯
 *
 * @details 구성:
 *  - 아이템 아이콘 이미지
 *  - 등급별 테두리 (Border 컴포넌트 색상으로 구분)
 *  - 중복 여부 텍스트 ("중복")
 *  - 환산 재화 수량 텍스트 (예: 200)
 */
UCLASS()
class PARADISE_API UParadiseGachaResultSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
	// ─────────────────────────────────────────────────────────────
#pragma region 외부 인터페이스
public:
	/**
	 * @brief 슬롯에 표시할 가챠 결과 데이터를 주입합니다.
	 * @param InResult          표시할 가챠 결과
	 * @param InRarityBorderMap 등급별 테두리 텍스처 맵 (부모 위젯에서 전달)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha|UI")
	void SetSlotData(
		const FGachaResult& InResult,
		const TMap<EItemRarity, UTexture2D*>& InRarityBorderMap);
#pragma endregion 외부 인터페이스

	// ─────────────────────────────────────────────────────────────
#pragma region UI 컴포넌트 (BindWidget)
protected:
	/** @brief 아이템 아이콘 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_ItemIcon = nullptr;

	/** @brief 등급 테두리 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_RarityBorder = nullptr;

	/** @brief 중복 여부 표시 텍스트 ("중복") — 중복이 아니면 Collapsed */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Duplicate = nullptr;

	/** @brief 환산 재화 수량 텍스트 (예: "200") */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Value = nullptr;
#pragma endregion UI 컴포넌트 (BindWidget)

	// ─────────────────────────────────────────────────────────────
#pragma region BP 연동 이벤트
protected:
	/**
	 * @brief 슬롯 데이터가 세팅된 직후 블루프린트에서 추가 연출 처리 (등장 애니메이션 등)
	 * @param InResult 세팅된 결과 데이터
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Paradise|Gacha|UI")
	void OnSlotDataSet(const FGachaResult& InResult);
#pragma endregion BP 연동 이벤트
};
