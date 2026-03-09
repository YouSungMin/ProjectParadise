// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseSummonPanel.generated.h"

#pragma region 전방선언
class UButton;
class UImage;
class UTextBlock;
class UDataTable;
class ALobbyPlayerController;
class UParadiseGameInstance;
#pragma endregion 전방선언

/**
 * @brief 소환 시스템의 각 탭(캐릭터, 장비 등) 안에 들어갈 컨텐츠 위젯의 기저 클래스
 * @details 이 클래스는 오직 '특정 소환 페이지의 데이터 표시'만 담당합니다.
 */
UCLASS()
class PARADISE_API UParadiseSummonPanel : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 패널 데이터를 초기화하거나 갱신하는 함수 (데이터 주도적 설계)
	 * @details 상속받은 자식 클래스에서 구체적인 배너 이미지나 확률 정보를 세팅합니다.
	 */
	virtual void RefreshPanelData();
#pragma endregion 외부 인터페이스

#pragma region 데이터 및 에셋 설정
protected:
	/** * @brief 기획자가 에디터에서 할당할 이 패널의 배너 행(Row) 데이터
	 * @details FDataTableRowHandle을 사용해 테이블과 특정 배너 행을 동시에 선택합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Data", meta = (RowType = "/Script/Paradise.GachaBannerData"))
	FDataTableRowHandle BannerDataRow;
#pragma endregion 데이터 및 에셋 설정

#pragma region UI 컴포넌트
protected:
	/** @brief 1회 소환 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_SummonSingle = nullptr;

	/** @brief 10회 소환 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_SummonMulti = nullptr;

	/**
	 * @brief 천장까지 남은 횟수 텍스트 (예: "천장까지 47회")
	 * @details WBP에 이 이름으로 TextBlock을 만들어두면 자동으로 업데이트됩니다.
	 *          없어도 컴파일 에러 없음 (BindWidgetOptional)
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PityRemaining = nullptr;

	/**
	 * @brief 현재 쌓인 천장 스택 텍스트 (예: "33 / 80")
	 * @details 디버그 또는 UI 표기용. 없어도 무방.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PityStack = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 내부 로직
protected:
	UFUNCTION()
	virtual void OnSingleSummonClicked();

	UFUNCTION()
	virtual void OnMultiSummonClicked();

	/** @brief 천장 UI 텍스트를 현재 서브시스템 값으로 갱신합니다. */
	void RefreshPityUI();
private:
	/**
	 * @brief 공통 소환 요청 로직 분리
	 * @param DrawCount 소환 횟수
	 */
	void RequestSummonAction(int32 DrawCount);
#pragma endregion 내부 로직

#pragma region 내부 상태 및 캐싱
private:
	/** @brief 가챠 액션(시퀀스 카메라 연출) 요청을 위한 컨트롤러 약참조 */
	TWeakObjectPtr<ALobbyPlayerController> CachedPlayerController = nullptr;

	/** @brief 서브시스템(GachaSubsystem) 접근용 게임 인스턴스 약참조 */
	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;
#pragma endregion 내부 상태 및 캐싱
};
