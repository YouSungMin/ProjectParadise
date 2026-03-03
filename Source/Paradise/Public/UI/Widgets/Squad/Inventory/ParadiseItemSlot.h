// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseItemSlot.generated.h"

#pragma region 전방 선언
class UImage;
class UButton;
#pragma endregion 전방 선언

/** @brief 슬롯 클릭 시 데이터와 함께 메인 위젯으로 알림 (부모/자식 공용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSlotBaseClicked, FSquadItemUIData, ItemData);

/**
 * @class UParadiseItemSlotBase
 * @brief 모든 인벤토리 슬롯의 기반이 되는 부모 위젯 클래스
 * @details 공통 UI 요소(아이콘, 테두리, 장착 마크)를 관리하며, 자식 클래스에서 UpdateSlot을 오버라이드하여 확장합니다.
 */
UCLASS(Abstract)
class PARADISE_API UParadiseItemSlot : public UUserWidget
{
	GENERATED_BODY()

#pragma region 생명주기 및 가상 함수
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * @brief 데이터를 받아 공통 UI(아이콘, 테두리)를 갱신합니다.
	 * @details 자식 클래스에서 이 함수를 오버라이드한 뒤, 반드시 Super::UpdateSlot()을 먼저 호출해야 합니다.
	 * @param InData 표시할 데이터 구조체
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Slot")
	virtual void UpdateSlot(const FSquadItemUIData& InData);
#pragma endregion 생명주기 및 가상 함수

#pragma region 내부 로직
protected:
	/** @brief 버튼 클릭 핸들러 (자식에서도 공유) */
	UFUNCTION()
	virtual void OnButtonClicked();

	/** @brief 등급 태그에 따라 테두리 색상을 변경 */
	void UpdateRankColor(EItemRarity Rarity);
#pragma endregion 내부 로직

#pragma region 공통 UI 바인딩
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Icon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_RankBorder = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_EquippedMark = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Select = nullptr;
#pragma endregion 공통 UI 바인딩

#pragma region 데이터 (Config)
protected:
	/** @brief 등급(Enum)에 따른 테두리 색상 맵 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI", meta = (DisplayName = "등급별 색상 맵"))
	TMap<EItemRarity, FLinearColor> RankColorMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI", meta = (DisplayName = "기본 테두리 색상"))
	FLinearColor DefaultRankColor = FLinearColor::White;

	FSquadItemUIData CachedData;
#pragma endregion 데이터 (Config)

#pragma region 델리게이트
public:
	/** @brief 클릭 이벤트 전파 델리게이트 (다형성을 위해 부모에 선언) */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemSlotBaseClicked OnSlotClicked;
#pragma endregion 델리게이트
};
