// Copyright (C) Project Paradise. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/Ingame/Popup/GameResultWidgetBase.h"
#include "UI/Widgets/Ingame/Result/ResultCharacterSlotWidget.h"
#include "VictoryPopupWidget.generated.h"

#pragma region 전방 선언
class UTextBlock;
class UImage;
class UTexture2D;
class UResultCharacterPanelWidget;
class UFamiliarRewardPopupWidget;
#pragma endregion 전방 선언

/**
 * @class UVictoryPopupWidget
 * @brief 게임 승리 시 표시되는 팝업 위젯.
 * @details
 * 1. 보상(골드, 경험치, 별)을 표시합니다.
 * 2. 캐릭터 목록 표시는 하위 컴포넌트인 ResultCharacterPanelWidget에게 위임합니다.
 * 3. 다음 스테이지 ID를 캐싱하여, 클릭 시 데이터테이블을 조회해 알맞은 맵으로 이동시킵니다.
 */
UCLASS()
class PARADISE_API UVictoryPopupWidget : public UGameResultWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 승리 데이터를 UI에 반영하고 상태를 캐싱합니다.
	 * @param InStageName 현재 스테이지 이름 (UI 표시용)
	 * @param InStarCount 획득한 별 개수 (1~3)
	 * @param InEarnedGold 이번 판에 획득한 골드
	 * @param InEarnedAether 이번 판에 획득한 에테르(최초 클리어 보상)
	 * @param InCharacterResults 캐릭터별 경험치 정산 데이터 배열
	 * @param InNextStageID 다음으로 이동할 스테이지의 고유 ID (테이블 조회용)
	 * @param InAcquiredFamiliar 이번 판에 획득한 퍼밀리어(3별 클리어 보상)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Result")
	void SetVictoryData(
		FText InStageName,
		int32 InStarCount,
		int32 InEarnedGold,
		int32 InEarnedAether,
		const TArray<FResultCharacterData>& InCharacterResults,
		FName InNextStageID,
		FName InAcquiredFamiliar);
#pragma endregion 외부 인터페이스

#pragma region 승리 전용 UI
protected:
	/** @brief 캐릭터 슬롯들을 관리하는 패널 위젯 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UResultCharacterPanelWidget> WBP_CharacterResultPanel = nullptr;

	/** @brief 다음 스테이지로 이동 버튼. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_NextStage = nullptr;

	/** @brief 획득 골드 표시 텍스트. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GoldValue = nullptr;

	/** @brief 획득 보석 표시 텍스트. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_AetherValue = nullptr;

	/** @brief 스테이지 이름 텍스트. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Stage = nullptr;

	/** @brief 첫 번째 별 이미지. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Star1 = nullptr;

	/** @brief 두 번째 별 이미지. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Star2 = nullptr;

	/** @brief 세 번째 별 이미지. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Star3 = nullptr;

	/** @brief 승리 팝업 등장 애니메이션 */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_PopupAppear = nullptr; // ⭐ 이 줄을 추가하세요!

	/**
	 * @brief 초회 3별 클리어 시 표시되는 퍼밀리어 보상 위젯
	 * @details AcquiredFamiliar가 None이면 숨김, 있으면 등장 애니메이션 재생
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UFamiliarRewardPopupWidget> WBP_FamiliarRewardPopup = nullptr;
#pragma endregion 승리 전용 UI

#pragma region 리소스 설정 (Data-Driven)
protected:
	/** @brief 별 활성화 시 사용할 텍스처 (노란 별). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paradise|Resource")
	TObjectPtr<UTexture2D> StarOnTexture = nullptr;

	/** @brief 별 비활성화 시 사용할 텍스처 (회색 별). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paradise|Resource")
	TObjectPtr<UTexture2D> StarOffTexture = nullptr;
#pragma endregion 리소스 설정

#pragma region 내부 로직
private:
	/** @brief 다음 스테이지 버튼 클릭 핸들러. */
	UFUNCTION()
	void OnNextStageClicked();

	/**
	 * @brief 별 이미지 위젯에 획득 여부에 따라 텍스처를 세팅합니다.
	 * @param StarImage 텍스처를 교체할 이미지 위젯
	 * @param StarIndex 해당 별의 순서 (1-based)
	 * @param InStarCount 실제 획득한 별 개수
	 */
	void SetStarImage(UImage* StarImage, int32 StarIndex, int32 InStarCount);

	/** @brief GameMode로부터 전달받은 다음 스테이지의 식별자(ID) */
	FName CachedNextStageID = NAME_None;
#pragma endregion 내부 로직
};