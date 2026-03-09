// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseChapterSelectWidget.generated.h"

#pragma region 전방 선언
class UScrollBox;
class UDataTable;
class UParadiseChapterSlotWidget;
class UParadiseGameInstance;
#pragma endregion 전방 선언

/**
 * @class UParadiseChapterSelectWidget
 * @brief 로비 우측에 나타나는 챕터 선택 세로 스크롤 목록 위젯
 * @details MVC 패턴 중 View를 담당하며, 데이터(Model)를 읽어 하위 슬롯 위젯들을 동적으로 생성합니다.
 */
UCLASS()
class PARADISE_API UParadiseChapterSelectWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region UI 컴포넌트
protected:
	/** @brief 챕터 슬롯들이 동적으로 추가될 세로 스크롤 박스 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> Scroll_ChapterList = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 데이터 설정
protected:
	/** * @brief 챕터 마스터 데이터 테이블
	 * @details 기획자가 엑셀로 작성한 FChapterData 구조체 기반의 데이터 테이블입니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Data")
	TObjectPtr<UDataTable> DT_ChapterData = nullptr;

	/** @brief 생성할 슬롯 위젯 블루프린트 클래스 (WBP_ChapterSlot) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI")
	TSubclassOf<UParadiseChapterSlotWidget> ChapterSlotClass = nullptr;
#pragma endregion 데이터 설정

#pragma region 내부 로직
private:
	/** @brief 챕터 리스트를 동적으로 생성하고 UI에 배치합니다. (최적화) */
	void BuildChapterList();

	/** * @brief 스테이지 서브시스템을 통해 해당 챕터의 해금 여부를 확인합니다. (캡슐화)
	 * @param RequiredStageID 챕터가 열리기 위해 필요한 첫 번째 스테이지 ID
	 * @return 해금 완료 여부
	 */
	bool IsChapterUnlocked(FName RequiredStageID) const;

	/** @brief 게임 인스턴스 약참조 캐싱 (메모리 릭 및 순환 참조 방지) */
	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;
#pragma endregion 내부 로직
};
