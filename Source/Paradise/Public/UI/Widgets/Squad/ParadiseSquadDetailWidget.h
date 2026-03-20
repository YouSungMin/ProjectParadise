// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseSquadDetailWidget.generated.h"

#pragma region 전방 선언
class UTextBlock;
class URichTextBlock;
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
	
#pragma region 생명주기
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 전달받은 데이터를 기반으로 UI 요소를 렌더링합니다. (Data-Driven)
	 * @param InData 표시할 UI 데이터 구조체
	 * @param InContext 현재 호출된 문맥 (편성창 캐릭터, 인벤토리 무기 등)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|DetailView")
	void ShowInfo(const FSquadItemUIData& InData, ESquadDetailContext InContext);

	/**
	 * @brief 현재 상태에 따라 하단 버튼부의 가시성을 동적으로 갱신합니다.
	 * @param CurrentState 현재 UI 상태 (Normal / EquipMode)
	 * @param bIsUnitTab 현재 유닛(소환수) 탭인지 여부
	 * @param bHasPendingSelection 교체할 대상이 선택되었는지 (확인 버튼 활성화용)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|DetailView")
	void UpdateButtonState(ESquadUIState CurrentState, bool bIsUnitTab, bool bHasPendingSelection);

	/** @brief 패널의 모든 정보를 비우고 숨깁니다. */
	void ClearInfo();
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/**
	 * @brief 캐릭터 장착 맵에서 특정 장비의 아이콘을 조회하여 동기 로드 및 렌더링합니다. (캡슐화)
	 * @param InSlot 조회할 장비 부위
	 * @param TargetImage 렌더링 대상 이미지 컴포넌트
	 * @param EquipmentMap 캐릭터의 현재 장비 장착 상태 맵
	 */
	void UpdateEquipmentIcon(EEquipmentSlot InSlot, UImage* TargetImage, const TMap<EEquipmentSlot, FGuid>& EquipmentMap);
#pragma endregion 내부 로직

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

	/** @brief 대상 스탯 (리치 텍스트 블록으로 변경하여 색상 태그 지원) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> Text_Desc = nullptr;

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

	/**
	 * @brief 부위별 빈 장비 슬롯 기본 배경 아이콘 맵
	 * @details 디자이너가 에디터에서 장비 부위(Key)와 기본 이미지(Value)를 자유롭게 매핑합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI|Fallback")
	TMap<EEquipmentSlot, TObjectPtr<UTexture2D>> DefaultEquipmentIcons;
#pragma endregion 기본 에셋 (Fallback)

#pragma region 델리게이트 이벤트
public:
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDetailAction OnSwapCharacterClicked;
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDetailAction OnSwapEquipmentClicked;
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDetailAction OnCancelClicked;
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDetailAction OnConfirmClicked;
#pragma endregion 델리게이트 이벤트
};
