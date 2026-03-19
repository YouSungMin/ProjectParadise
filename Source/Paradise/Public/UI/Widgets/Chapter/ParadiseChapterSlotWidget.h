// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseChapterSlotWidget.generated.h"

#pragma region 전방 선언
class UButton;
class UTextBlock;
class UMaterialInterface;
#pragma endregion 전방 선언

/**
 * @brief 챕터 슬롯 클릭 델리게이트
 * @param ChapterID  클릭된 챕터 번호
 * @param MapTexture 해당 챕터의 3D 지도 배경 텍스처
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChapterSelected, int32, ChapterID, UTexture2D*, MapTexture);

/**
 * @class UParadiseChapterSlotWidget
 * @brief 챕터 리스트의 개별 슬롯 위젯 (예: 챕터 1 버튼)
 */
UCLASS()
class PARADISE_API UParadiseChapterSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 챕터 슬롯의 시각적 요소를 초기화합니다. (Data-Driven)
	 * @param InChapterID 챕터 고유 번호
	 * @param bIsUnlocked 해당 챕터의 해금(입장 가능) 여부
	 * @param InMapTexture 챕터 진입 시 배경으로 깔릴 3D 지도 텍스처
	 * @param InButtonMaterial 버튼 표면을 덮을 전용 머티리얼
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Chapter")
	void InitSlot(int32 InChapterID, bool bIsUnlocked, UTexture2D* InMapTexture, UMaterialInterface* InButtonMaterial);

	/**
	 * @brief 챕터 버튼이 클릭됐을 때 발사되는 델리게이트
	 * @details 부모 위젯(ChapterSelectWidget) 이 바인딩하여 컨트롤러에 전달합니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Chapter")
	FOnChapterSelected OnChapterSelected;
#pragma endregion 외부 인터페이스

#pragma region UI 컴포넌트
protected:
	/** @brief 챕터 진입 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Chapter = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 내부 로직
private:
	/** @brief 버튼 클릭 처리 */
	UFUNCTION()
	void OnChapterClicked();

	/** @brief 현재 할당된 챕터 ID */
	int32 CurrentChapterID = 1;

	/** @brieft 전달받은 텍스처를 들고 있을 변수 추가 */
	UPROPERTY()
	TObjectPtr<UTexture2D> CachedMapTexture = nullptr;
#pragma endregion 내부 로직
};
