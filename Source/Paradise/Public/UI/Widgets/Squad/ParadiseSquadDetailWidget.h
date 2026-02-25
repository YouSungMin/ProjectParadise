// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseSquadDetailWidget.generated.h"

#pragma region 전방 선언
class UTextBlock;
class UButton;
class UHorizontalBox;
class UImage;
#pragma endregion 전방 선언

/** @brief 상세창 액션 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDetailAction);

/**
 * @class UParadiseSquadDetailWidget
 * @brief 편성 화면 좌측 하단 상세 정보 패널. 상태에 따라 교체/장비 버튼 또는 확인/취소 버튼을 표시함.
 */
UCLASS()
class PARADISE_API UParadiseSquadDetailWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 로직
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * @brief 받은 데이터를 UI에 표시합니다.
	 * @param InData 표시할 UI 데이터 구조체
	 * @param bIsFormationContext [수정] 편성창에서 클릭했는지 여부 (true: 버튼 보임, false: 버튼 숨김)
	 * @param bIsUnit 유닛인지 여부 (true: 장비버튼 숨김)
	 */
	UFUNCTION(BlueprintCallable, Category = "DetailView")
	void ShowInfo(const FSquadItemUIData& InData, bool bIsFormationContext, bool bIsUnit);

	/**
	 * @brief 현재 상태에 따라 버튼의 가시성을 갱신합니다.
	 * @param CurrentState 현재 UI 상태 (Normal / EquipMode)
	 * @param bIsUnitTab 현재 유닛(소환수) 탭인지 여부
	 * @param bHasPendingSelection 교체할 대상이 선택되었는지 (확인 버튼 활성화용)
	 */
	UFUNCTION(BlueprintCallable, Category = "DetailView")
	void UpdateButtonState(ESquadUIState CurrentState, bool bIsUnitTab, bool bHasPendingSelection);

	/** @brief 정보를 비웁니다. */
	void ClearInfo();
#pragma endregion 로직

#pragma region 핸들러
private:
	UFUNCTION() 
	void HandleSwapChar();
	UFUNCTION() 
	void HandleSwapEquip();
	UFUNCTION() 
	void HandleCancel();

	// 확인 버튼 핸들러
	UFUNCTION()
	void HandleConfirm();
#pragma endregion 핸들러

#pragma region UI 바인딩
protected:
	/** @brief 대상 이름 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Name = nullptr;

	/** @brief 대상 설명 또는 스탯 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Desc = nullptr;

	/** @brief 대상 이미지 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon = nullptr;

	/** @brief 캐릭터 교체 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_SwapCharacter = nullptr;

	/** @brief 장비 교체 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_SwapEquipment = nullptr;

	/** @brief 장비 교체 모드 취소/완료 버튼 (초기엔 숨김) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_CancelEquipMode = nullptr;

	// 교체 확정 버튼 (교체 모드에서만 보임, 처음엔 비활성)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Confirm = nullptr;

	// 버튼들을 담는 가로 박스 (인벤토리 클릭 시 통째로 숨기기 위함)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HBox_ButtonRoot = nullptr;
#pragma endregion UI 바인딩

#pragma region 이벤트
public:
	/** @brief 캐릭터 교체 버튼 클릭 알림 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDetailAction OnSwapCharacterClicked;

	/** @brief 장비 교체 버튼 클릭 알림 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDetailAction OnSwapEquipmentClicked;

	/** @brief 취소 버튼 클릭 알림 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDetailAction OnCancelClicked;

	// 확인 버튼 클릭 알림
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDetailAction OnConfirmClicked;
#pragma endregion 이벤트

private:
	/** @brief 현재 보여주는 정보가 편성(Formation) 슬롯의 정보인지 여부 */
	bool bIsFormation = false;
};
