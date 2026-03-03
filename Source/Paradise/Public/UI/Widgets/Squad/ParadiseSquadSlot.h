// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseSquadSlot.generated.h"

#pragma region 전방 선언
class UImage;
class UButton;
class UTextBlock;
class USizeBox;
#pragma endregion 전방 선언

/** @brief 슬롯 클릭 시 인덱스 전달 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSquadSlotClicked, int32, Index);

/**
 * @class UParadiseSquadSlot
 * @brief 편성 화면(좌측)에 배치되는 개별 슬롯 (메인/서브/유닛 공용)
 * @details 비어있는 상태(Empty)와 채워진 상태를 구분하며, 선택 시 하이라이트 효과를 줍니다.
 */
UCLASS()
class PARADISE_API UParadiseSquadSlot : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 로직
public:
	/** @brief 에디터 편집 화면 및 런타임 초기화 시 크기 적용 */
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * @brief 슬롯 초기화 (인덱스 지정)
	 * @details 위젯 생성 직후 호출해 주세요.
	 */
	UFUNCTION(BlueprintCallable, Category = "SquadSlot")
	void InitSlot(int32 InSlotIndex);

	/**
	 * @brief 데이터로 슬롯 갱신
	 * @param InData 데이터 (ID가 None이면 비어있는 것으로 처리)
	 */
	UFUNCTION(BlueprintCallable, Category = "SquadSlot")
	void UpdateSlot(const FSquadItemUIData& InData);

	/** @brief 선택 상태(하이라이트) 설정 */
	UFUNCTION(BlueprintCallable, Category = "SquadSlot")
	void SetSelected(bool bIsSelected);

private:
	UFUNCTION()
	void OnButtonClicked();
#pragma endregion 로직

#pragma region 설정 (Config)
public:
	/** @brief 슬롯 가로 크기 (에디터 디테일 패널에서 수정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float SlotWidth = 150.0f;

	/** @brief 슬롯 세로 크기 (에디터 디테일 패널에서 수정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float SlotHeight = 150.0f;
#pragma endregion 설정

#pragma region UI 바인딩
protected:
	/** @brief 최상위 루트 사이즈 박스 (크기 강제용) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> RootSizeBox = nullptr;

	/** @brief 캐릭터/유닛 아이콘 (채워졌을 때 보임) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Icon = nullptr;

	/** @brief 비어있을 때 보여줄 아이콘 (+ 표시 등) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_EmptyPlaceholder = nullptr;

	/** @brief 선택되었을 때 켜지는 테두리/하이라이트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SelectionBorder = nullptr;

	/** @brief 레벨 표시 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Level = nullptr;

	/** @brief 클릭 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Select = nullptr;
#pragma endregion UI 바인딩

#pragma region 내부 상태
private:
	/** @brief 이 슬롯의 고유 인덱스 (0:Main, 1:Sub1, 2:Sub2, 3~7:Units) */
	int32 SlotIndex = -1;

	/** @brief 현재 데이터가 비어있는지 여부 */
	bool bIsEmpty = true;
#pragma endregion 내부 상태

#pragma region 델리게이트
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSquadSlotClicked OnSlotClicked;
#pragma endregion 델리게이트
};
