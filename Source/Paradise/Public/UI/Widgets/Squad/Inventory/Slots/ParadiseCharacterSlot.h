// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseCharacterSlot.generated.h"

#pragma region 전방 선언
class UImage;
class UTextBlock;
class UButton;
#pragma endregion 전방 선언

/** @brief 슬롯 클릭 시 데이터와 함께 알림 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSlotClicked, FSquadItemUIData, ItemData);

/**
 * @class UParadiseCharacterSlot
 * @brief 인벤토리 내 캐릭터 전용 슬롯 (수량 표시 제외)
 */
UCLASS()
class PARADISE_API UParadiseCharacterSlot : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 로직
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * @brief 데이터를 받아 UI를 갱신합니다.
	 * @param InData 표시할 캐릭터 데이터 구조체
	 */
	UFUNCTION(BlueprintCallable, Category = "Slot|Character")
	void UpdateSlot(const FSquadItemUIData& InData);

private:
	UFUNCTION()
	void OnButtonClicked();

	void UpdateRankColor(FGameplayTag RankTag);
#pragma endregion 로직

#pragma region UI 바인딩
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Icon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_RankBorder = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Level = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_EquippedMark = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Select = nullptr;
#pragma endregion UI 바인딩

#pragma region 데이터 (Config)
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI", meta = (DisplayName = "등급별 색상 맵"))
	TMap<FGameplayTag, FLinearColor> RankColorMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI", meta = (DisplayName = "기본 테두리 색상"))
	FLinearColor DefaultRankColor = FLinearColor::White;
#pragma endregion 데이터 (Config)

#pragma region 내부 상태
private:
	FSquadItemUIData CachedData;
#pragma endregion 내부 상태

#pragma region 델리게이트
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCharacterSlotClicked OnSlotClicked;
#pragma endregion 델리게이트
};
