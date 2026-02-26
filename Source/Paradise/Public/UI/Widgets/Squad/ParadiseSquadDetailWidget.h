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
class UWidget;
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
	void ShowInfo(const FSquadItemUIData& InData, ESquadDetailContext Context);

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

private:
	/**
	 * @brief 캐릭터의 장비 맵에서 특정 부위의 장착 아이템 아이콘을 찾아 UI에 세팅합니다.
	 * @param InSlot 찾고자 하는 장비 부위 (Weapon, Helmet 등)
	 * @param TargetImage 세팅할 대상 UI 이미지 컴포넌트
	 * @param EquipmentMap 캐릭터가 현재 장착 중인 아이템 GUID 맵
	 */
	void UpdateEquipmentIcon(EEquipmentSlot InSlot, UImage* TargetImage, const TMap<EEquipmentSlot, FGuid>& EquipmentMap);

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

#pragma region UI 바인딩 (공통)
protected:
	/** @brief 대상 이름 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Name = nullptr;

	/** @brief 대상 스탯 및 상세 설명  */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Desc = nullptr;

	/** @brief 메인 아이콘 이미지 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon = nullptr;

	/** @brief 하단 액션 버튼 그룹 (편성창 클릭 시에만 활성화) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HBox_ButtonRoot = nullptr;
#pragma endregion UI 바인딩 (공통)

#pragma region UI 바인딩 (동적 레이아웃 영역)
	/** * @brief [편성 캐릭터 전용] 장착 중인 5개의 장비(무기1, 방어구/악세4)를 묶어둔 컨테이너
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Container_EquippedItems = nullptr;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_EquipWeapon = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_EquipHelmet = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_EquipChest = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_EquipAcc1 = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_EquipAcc2 = nullptr;

	/** * @brief [캐릭터 궁극기 & 무기 스킬 전용] 스킬 정보를 묶어둔 컨테이너
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Container_Skill = nullptr;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_SkillIcon = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_SkillInfo = nullptr;
#pragma endregion UI 바인딩 (동적 레이아웃 영역)

#pragma region UI 바인딩 (버튼)
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_SwapCharacter = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_SwapEquipment = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_CancelEquipMode = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Confirm = nullptr;
#pragma endregion UI 바인딩 (버튼)

#pragma region 기본 에셋 (Fallback)
protected:
	/**
	 * @brief 데이터 테이블에 메인 아이콘 이미지가 없을 때 띄워줄 기본(물음표/실루엣) 아이콘
	 * @details 블루프린트 에디터 우측 디테일 패널에서 디자이너가 직접 세팅합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI|Fallback")
	TObjectPtr<UTexture2D> DefaultMainIcon = nullptr;

	/** @brief 장착된 장비 이미지가 없을 때 빈칸 대신 채워줄 기본 아이콘 (선택 사항) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI|Fallback")
	TObjectPtr<UTexture2D> DefaultEquipIcon = nullptr;
#pragma endregion 기본 에셋 (Fallback)

#pragma region 델리게이트 이벤트
public:
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDetailAction OnSwapCharacterClicked;
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDetailAction OnSwapEquipmentClicked;
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDetailAction OnCancelClicked;
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDetailAction OnConfirmClicked;
#pragma endregion 델리게이트 이벤트
};
