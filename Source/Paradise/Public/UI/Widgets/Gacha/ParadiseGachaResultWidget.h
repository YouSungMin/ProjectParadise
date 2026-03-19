// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Structs/GachaTypes.h"
#include "ParadiseGachaResultWidget.generated.h"

#pragma region 전방 선언
class UButton;
class UWidgetSwitcher;
class UUniformGridPanel;
class UImage;
class UTextBlock;
class UTexture2D;
class ALobbyPlayerController;
class UParadiseGachaResultSlotWidget;
#pragma endregion 전방 선언

/**
 * @class UParadiseGachaResultWidget
 * @brief 가챠 연출이 모두 끝난 후 결과를 보여주는 최종 결과창
 *
 * @details 레이아웃:
 *  [1연차] WidgetSwitcher → INDEX_SINGLE
 *    - 세로로 긴 카드 1장 (캐릭터 일러스트 + 등급 테두리 + 이름 + 스탯)
 *
 *  [10연차] WidgetSwitcher → INDEX_MULTI
 *    - UniformGridPanel 5열 2행, 슬롯 10개 동적 생성
 *    - 각 슬롯: 아이콘 + 등급 테두리 + 중복 텍스트 + 환산 재화 수량
 *
 *  하단 중앙: 계속(확인) 버튼 → 소환 패널로 복귀
 */
UCLASS()
class PARADISE_API UParadiseGachaResultWidget : public UUserWidget
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
	 * @brief Controller로부터 최종 뽑기 결과를 전달받아 개수에 맞게 화면을 세팅합니다.
	 * @param Results 1연차 또는 10연차의 최종 결과 배열
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha")
	void ShowResults(const TArray<FGachaResult>& Results);
#pragma endregion 외부 인터페이스

#pragma region UI 이벤트 (BP 연동용)
protected:
	/**
	 * @brief 1연차 전용 UI 갱신 (블루프린트에서 상세 정보, 전신 일러스트 바인딩)
	 * @param Result 단일 가챠 결과 데이터
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Paradise|Gacha|UI")
	void OnDisplaySingleResult(const FGachaResult& Result);

	/**
	 * @brief 10연차 전용 UI 갱신 (블루프린트에서 니케 스타일 10칸 슬롯 바인딩)
	 * @param Results 10개의 가챠 결과 배열
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Paradise|Gacha|UI")
	void OnDisplayMultiResult(const TArray<FGachaResult>& Results);
#pragma endregion UI 이벤트 (BP 연동용)

#pragma region UI 컴포넌트
protected:
	/** @brief 1회 뷰 / 10회 뷰 전환 스위처 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_ResultLayout = nullptr;

	/** @brief 하단 중앙 계속(확인) 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Confirm = nullptr;

	// ── 1연차 전용 컴포넌트 ─────────────────────────────────

	/** @brief 1연차 카드 아이템 아이콘/일러스트 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SingleItemIcon = nullptr;

	/** @brief 1연차 카드 등급 테두리 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SingleRarityBorder = nullptr;

	/** @brief 1연차 카드 아이템 이름 텍스트 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SingleItemName = nullptr;

	// ── 10연차 전용 컴포넌트 ────────────────────────────────

	/** @brief 10연차 슬롯 5열 2행 그리드 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> Grid_MultiResult = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 에셋 설정 
protected:
	/**
	 * @brief 등급별 테두리 텍스처 맵
	 * @details Common ~ Legendary 각 등급에 맞는 테두리 텍스처를 연결합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Gacha|Visual")
	TMap<EItemRarity, TObjectPtr<UTexture2D>> RarityBorderTextureMap;

	/** @brief 10연차 슬롯 위젯 블루프린트 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Gacha|Visual")
	TSubclassOf<UParadiseGachaResultSlotWidget> SlotWidgetClass;

	/** @brief 10연차 그리드 열 수 (기본 5열 — 2행 자동) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Gacha|Visual", meta = (ClampMin = "1", ClampMax = "10"))
	int32 GridColumnCount = 5;
#pragma endregion 에셋 설정 

#pragma region 내부 로직
private:
	/** @brief 1연차 카드 UI 세팅 */
	void SetupSingleResult(const FGachaResult& Result);

	/** @brief 10연차 그리드 슬롯 동적 생성 및 세팅 */
	void SetupMultiResult(const TArray<FGachaResult>& Results);

	/** @brief 확인 버튼 클릭 핸들러 */
	UFUNCTION()
	void OnConfirmClicked();
#pragma endregion 내부 로직

#pragma region 내부 상태 및 캐싱
private:
	/** @brief 스위처 인덱스 하드코딩 방지 상수 */
	static constexpr int32 INDEX_SINGLE = 0;
	static constexpr int32 INDEX_MULTI = 1;

	/** @brief 소환 패널 복귀용 컨트롤러 약참조 */
	TWeakObjectPtr<ALobbyPlayerController> CachedPlayerController = nullptr;
#pragma endregion 내부 상태 및 캐싱

};
