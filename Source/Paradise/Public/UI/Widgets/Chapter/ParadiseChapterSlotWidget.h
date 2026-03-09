// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseChapterSlotWidget.generated.h"

#pragma region 전방 선언
class UButton;
class UTextBlock;
class ALobbyPlayerController;
#pragma endregion 전방 선언

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
	 * @param InChapterName UI에 표시될 챕터 이름
	 * @param bIsUnlocked 해당 챕터의 해금(입장 가능) 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Chapter")
	void InitSlot(int32 InChapterID, const FText& InChapterName, bool bIsUnlocked, UTexture2D* InMapTexture);
#pragma endregion 외부 인터페이스

#pragma region UI 컴포넌트
protected:
	/** @brief 챕터 진입 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Chapter = nullptr;

	/** @brief 챕터 이름 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ChapterName = nullptr;

	/** @brief 잠김 표시 텍스트 또는 아이콘 ("잠김") */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_LockStatus = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 내부 로직
private:
	/** @brief 버튼 클릭 처리 */
	UFUNCTION()
	void OnChapterClicked();

	/** @brief 현재 할당된 챕터 ID */
	int32 CurrentChapterID = 1;

	/** @brief 컨트롤러 약참조 캐싱 */
	TWeakObjectPtr<ALobbyPlayerController> CachedController = nullptr;

	/** @brieft 전달받은 텍스처를 들고 있을 변수 추가 */
	UPROPERTY()
	TObjectPtr<UTexture2D> CachedMapTexture = nullptr;
#pragma endregion 내부 로직
};
