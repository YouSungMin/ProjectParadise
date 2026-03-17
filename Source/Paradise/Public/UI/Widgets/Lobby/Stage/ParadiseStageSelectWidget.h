// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseStageSelectWidget.generated.h"

#pragma region 전방 선언
class UWidgetSwitcher;
//class UCanvasPanel;
class UDataTable;
class UButton;
class UImage;
class UParadiseStageDetailWidget;
class UParadiseGameInstance;
#pragma endregion 전방 선언

/**
 * @class UParadiseStageSelectWidget
 * @brief 스테이지 선택 화면 메인 위젯.
 * @details 두 개의 데이터 테이블(Stats, Assets)을 읽어 리스트를 초기화합니다.
 */
UCLASS()
class PARADISE_API UParadiseStageSelectWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma region UI 컴포넌트
protected:
	/**
	 * @brief 챕터에 따라 동적으로 바뀔 지도 배경 이미지
	 * @details Z-Order를 가장 낮게(계층구조 맨 위) 설정하여 노드들 뒤에 깔리도록 해야 합니다.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_MapBackground = nullptr;

	/**
	 * @brief 챕터별 캔버스들을 담아두고 교체해 줄 스위처
	 * @details 인덱스 0 = 1챕터 캔버스, 인덱스 1 = 2챕터 캔버스 ...
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_ChapterMaps = nullptr; // 🚨 [교체]

	/** @brief 로비로 돌아가는 뒤로가기 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Back = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 컨트롤러에서 호출하여 현재 챕터 번호에 맞는 스테이지 노드들을 세팅합니다.
	 * @param InChapterID 현재 진입한 챕터 번호
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Stage")
	void InitStageMap(int32 InChapterID);
#pragma endregion 외부 인터페이스

#pragma region 데이터 (Data)
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Data")
	TObjectPtr<UDataTable> DT_StageStats = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Data")
	TObjectPtr<UDataTable> DT_StageAssets = nullptr;

	/**
	 * @brief  상세 정보 팝업 창 (WBP_StageSelect 내부에 배치)
	 * @details 에디터에서 WBP_StageDetail 위젯을 캔버스에 올리고 이름을 'UI_StageDetail'로 맞춰야 합니다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseStageDetailWidget> UI_StageDetail = nullptr;
#pragma endregion 데이터 (Data)

private:
	/** @brief 뒤로가기 버튼이 눌렸을 때 호출 될 함수 */
	UFUNCTION()
	void OnClickBack();

	/** @brief 맵 노드들의 가시성을 갱신합니다. */
	void RefreshMapNodes();

	/** @brief (임시) 해당 스테이지가 해금되었는지 확인하는 헬퍼 함수 */
	bool IsStageUnlocked(FName StageID);

	/**
	 * @brief 자식 노드(StageNodeWidget)에서 브로드캐스트한 클릭 이벤트를 수신합니다.
	 * @param SelectedStageID 클릭된 노드의 스테이지 ID
	 */
	UFUNCTION()
	void HandleNodeClicked(FName SelectedStageID);

	/** @brief 팝업이 닫혔다는 신호를 받는 함수 */
	UFUNCTION()
	void HandleDetailClosed();

#pragma region 내부 캐싱
private:
	/** @brief 게임 인스턴스 1회 캐싱 */
	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;

	/** @brief 현재 선택된 챕터 번호 보관용 */
	int32 CurrentChapterID = 1;
#pragma endregion 내부 캐싱
};
