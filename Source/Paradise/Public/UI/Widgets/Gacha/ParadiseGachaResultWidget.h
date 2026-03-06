// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Structs/GachaTypes.h"
#include "ParadiseGachaResultWidget.generated.h"

#pragma region 전방 선언
class UButton;
class UWidgetSwitcher;
class ALobbyPlayerController;
#pragma endregion 전방 선언

/**
 * @class UParadiseGachaResultWidget
 * @brief 가챠 연출(3D 구슬 까기)이 모두 끝난 후 아이템을 보여주는 최종 결과창 (View)
 * @details 1회 소환(상세 정보)과 10회 소환(10칸 슬롯)의 UI 레이아웃을 스위처로 분기하여 표시합니다.
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
	/** * @brief 1연차 전용 UI 갱신 (블루프린트에서 상세 정보, 전신 일러스트 바인딩)
	 * @param Result 단일 가챠 결과 데이터
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Paradise|Gacha|UI")
	void OnDisplaySingleResult(const FGachaResult& Result);

	/** * @brief 10연차 전용 UI 갱신 (블루프린트에서 니케 스타일 10칸 슬롯 바인딩)
	 * @param Results 10개의 가챠 결과 배열
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Paradise|Gacha|UI")
	void OnDisplayMultiResult(const TArray<FGachaResult>& Results);
#pragma endregion UI 이벤트 (BP 연동용)

#pragma region UI 컴포넌트
protected:
	/** @brief 1회(상세 뷰) / 10회(슬롯 뷰) 패널을 전환하는 스위처 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_ResultLayout = nullptr;

	/** @brief '확인'을 누르고 다시 소환 메뉴(로비)로 돌아가는 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Confirm = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 내부 상태 및 캐싱
private:
	/** @brief 스위처 인덱스 하드코딩 방지 상수 */
	static constexpr int32 INDEX_SINGLE = 0;
	static constexpr int32 INDEX_MULTI = 1;

	/** @brief 화면 전환을 위한 컨트롤러 약참조 */
	TWeakObjectPtr<ALobbyPlayerController> CachedPlayerController = nullptr;
#pragma endregion 내부 상태 및 캐싱

#pragma region 내부 로직
private:
	/** @brief 확인 버튼 클릭 핸들러 */
	UFUNCTION()
	void OnConfirmClicked();
#pragma endregion 내부 로직
};
