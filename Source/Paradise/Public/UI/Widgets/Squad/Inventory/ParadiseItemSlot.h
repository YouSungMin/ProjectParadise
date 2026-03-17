// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseItemSlot.generated.h"

#pragma region 전방 선언
class UImage;
class UButton;
class UTexture2D;
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
	/** @brief 버튼 클릭 이벤트를 델리게이트로 브로드캐스트합니다. */
	UFUNCTION()
	void OnButtonClicked();

	/** @brief 아이템 본체 아이콘을 갱신합니다. */
	void UpdateMainIconUI();

	/** @brief 아이템 등급에 따른 테두리 색상 및 글자 엠블럼을 갱신합니다. */
	void UpdateRankUI();

	/** @brief 장착 여부에 따른 마크 UI를 갱신합니다. */
	void UpdateEquipStateUI();

	/** @brief 보유 여부에 따른 흑백/블러 처리 및 상호작용 활성화를 제어합니다. */
	void UpdateOwnershipStateUI();
#pragma endregion 내부 로직

#pragma region 공통 UI 바인딩
protected:
	/** @brief 아이템/캐릭터 메인 아이콘 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Icon = nullptr;

	/** @brief 등급별 색상이 적용될 테두리/배경 이미지 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_RankBorder = nullptr;

	/** @brief N, R, SR 등 등급 글자 엠블럼 이미지 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_RankIcon = nullptr;

	/** @brief 장착 중(E) 표시 마크 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_EquippedMark = nullptr;

	/** @brief 슬롯 전체 터치/클릭을 담당하는 투명 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Select = nullptr;
#pragma region 데이터 (Config)
protected:
	/** @brief 등급(Enum)에 따른 테두리 색상 매핑 테이블 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI|Config", meta = (DisplayName = "등급별 색상 맵"))
	TMap<EItemRarity, FLinearColor> RankColorMap;

	/** @brief 매핑된 색상이 없을 경우 사용할 기본 테두리 색상 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI|Config", meta = (DisplayName = "기본 테두리 색상"))
	FLinearColor DefaultRankColor = FLinearColor::White;

	/** @brief 등급(Enum)에 따른 N, R, SR 글자 엠블럼 텍스처 매핑 테이블 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI|Config", meta = (DisplayName = "등급별 엠블럼 아이콘 맵"))
	TMap<EItemRarity, TObjectPtr<UTexture2D>> RankIconMap;

	/** @brief 매핑된 엠블럼이 없을 경우 사용할 기본 아이콘 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI|Config", meta = (DisplayName = "기본 엠블럼 아이콘"))
	TObjectPtr<UTexture2D> DefaultRankIcon = nullptr;

	/** @brief 현재 UI가 렌더링하고 있는 데이터를 캡슐화하여 보관 */
	UPROPERTY(Transient)
	FSquadItemUIData CachedData;
#pragma endregion 데이터 (Config)

#pragma region 델리게이트
public:
	/** @brief 클릭 이벤트 전파 델리게이트 (다형성을 위해 부모에 선언) */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemSlotBaseClicked OnSlotClicked;
#pragma endregion 델리게이트
};
