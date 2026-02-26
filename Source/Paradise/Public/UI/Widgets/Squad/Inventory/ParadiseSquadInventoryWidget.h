// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseSquadInventoryWidget.generated.h"

#pragma region 전방 선언
class UWidgetSwitcher;
class UWrapBox;
class UScrollBox;
class UParadiseItemSlot;
class UParadiseCharacterSlot;   
class UParadiseEquipmentSlot;   
class UParadiseUnitSlot;        
#pragma endregion 전방 선언

/** @brief 아이템 클릭 시 메인 위젯으로 알림 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSquadItemClicked, FSquadItemUIData, ItemData);

/**
 * @class UParadiseSquadInventoryWidget
 * @brief 우측 인벤토리 리스트를 담당하는 뷰(View) 위젯
 * @details 데이터를 받아 WrapBox에 5개씩 나열합니다.
 */
UCLASS()
class PARADISE_API UParadiseSquadInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 공개 함수
public:
	/**
	 * @brief 리스트 데이터를 받아서 화면을 갱신합니다.
	 * @param TabIndex 보여줄 탭 인덱스
	 * @param ListData 그릴 데이터 목록
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad|View")
	void UpdateList(int32 TabIndex, const TArray<FSquadItemUIData>& ListData);
#pragma endregion 공개 함수

#pragma region 내부 로직
private:
	/** @brief 슬롯 클릭 핸들러 (내부 바인딩용) */
	UFUNCTION()
	void HandleSlotClick(FSquadItemUIData ItemData);
#pragma endregion 내부 로직

#pragma region UI 바인딩
protected:
	/** @brief 탭별 내용을 교체할 스위처 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_List = nullptr;

	/** @brief 캐릭터 리스트 랩박스 (ScrollBox 내부에 위치) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> Wrap_Character = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> Wrap_Weapon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> Wrap_Armor = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> Wrap_Unit = nullptr;
#pragma endregion UI 바인딩

#pragma region 슬롯 설정 데이터 (Config)
protected:
	/** @brief 캐릭터 탭에 표시할 전용 슬롯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Config")
	TSubclassOf<UParadiseCharacterSlot> CharacterSlotClass = nullptr;

	/** @brief 무기/방어구 탭에 표시할 전용 슬롯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Config")
	TSubclassOf<UParadiseEquipmentSlot> EquipmentSlotClass = nullptr;

	/** @brief 유닛 탭에 표시할 전용 슬롯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Config")
	TSubclassOf<UParadiseUnitSlot> UnitSlotClass = nullptr;
#pragma endregion 슬롯 설정 데이터 (Config)

#pragma region 이벤트 델리게이트
public:
	UPROPERTY(BlueprintAssignable)
	FOnSquadItemClicked OnItemClicked;
#pragma endregion 이벤트 델리게이트
};
