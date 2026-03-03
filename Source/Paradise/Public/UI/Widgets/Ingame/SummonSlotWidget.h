// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "SummonSlotWidget.generated.h"

#pragma region 전방 선언
class UParadiseCommonButton;
class UImage;
class UTextBlock;
class UWidgetAnimation;
class UTexture2D;
#pragma endregion 전방 선언

/** @brief 슬롯 클릭 시 인덱스를 전달하는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSummonSlotClicked, int32, SlotIndex);

/**
 * @class USummonSlotWidget
 * @brief 소환 패널의 개별 슬롯 UI (View)
 * @details 아이콘, 코스트 표시 및 등장 애니메이션(Pop-up)을 담당합니다.
 */
UCLASS()
class PARADISE_API USummonSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	USummonSlotWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 슬롯의 인덱스를 초기화합니다.
	 * @param InIndex 할당할 슬롯 번호
	 */
	void InitSlot(int32 InIndex);

	/**
	 * @brief 소환수 데이터를 받아 UI를 갱신합니다.
	 * @param IconTexture 표시할 아이콘 (nullptr 처리 포함)
	 * @param InCost 소환 비용
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void UpdateSlotInfo(UTexture2D* IconTexture, int32 InCost);

	/**
	 * @brief 딜레이를 가지고 슬롯 정보를 갱신하도록 예약합니다. (데이터 주도적 설계)
	 * @param IconTexture 예약할 아이콘
	 * @param InCost 예약할 소환 비용
	 * @param DelayTime 몇 초 뒤에 등장할 것인가
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void ScheduleReveal(UTexture2D* IconTexture, int32 InCost, float DelayTime);

	/**
	 * @brief 슬롯 등장 애니메이션을 재생합니다.
	 * @details 새로 추가된 슬롯임을 강조할 때 호출합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void PlayIntroAnimation();

	/**
	 * @brief 옆에서 당겨져 오는(Shift) 애니메이션을 재생합니다.
	 * @details 빈자리를 채울 때 사용됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Animation")
	void PlayShiftAnimation();

	///**
	// * @brief 쿨타임 상태를 갱신합니다. (GAS로부터 호출 권장)
	// * @param CurrentTime 남은 시간
	// * @param MaxTime 전체 쿨타임 (재설정 필요 시)
	// * @note 26/02/12, 지금은 안쓰는 코드
	// */
	//UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	//void RefreshCooldown(float CurrentTime, float MaxTime);
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/** @brief 버튼 클릭 시 발생할 이벤트 핸들러 */
	UFUNCTION()
	void OnSummonButtonClicked();

	/** @brief 예약된 지연 시간이 끝났을 때 호출되어 실제로 화면에 표시하는 핸들러 */
	UFUNCTION()
	void OnRevealTimerFinished();

	///**
	// * @brief 타이머에 의해 주기적으로 호출되어 쿨타임 UI를 갱신합니다.
	// * @note 26/02/12, 지금은 안쓰는 코드
	// */
	//UFUNCTION()
	//void UpdateCooldownVisual();

	///** @brief 쿨타임 UI를 숨기고 상태를 리셋합니다. */
	//UFUNCTION()
	//void StopCooldownTimer();
#pragma endregion 내부 로직

//#pragma region 쿨타임 설정
//private:
//	/** @brief 쿨타임 UI 업데이트 주기 (초)입니다. */
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
//	float UpdateInterval = 0.05f;
//#pragma endregion 쿨타임 설정

#pragma region 위젯 바인딩
private:
	/** @brief 클릭 입력을 담당하는 Common UI 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> Btn_SummonAction = nullptr;

	/** @brief 소환수 아이콘 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SummonIcon = nullptr;

	///** @brief 쿨타임 진행바 */
	///  * @note 26/02/12, 지금은 안쓰는 코드
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UProgressBar> PB_Cooldown = nullptr;

	///** @brief 남은 시간을 표시할 텍스트 */
	///  * @note 26/02/12, 지금은 안쓰는 코드
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UTextBlock> Text_CooldownTime = nullptr;

	/** @brief 유닛의 코스트를 표시할 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CostValue = nullptr;

	/**
	 * @brief 등장 애니메이션 (에디터에서 이름 'Anim_Intro'로 생성 필수)
	 * @details 오른쪽에서 왼쪽으로 이동
	 */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_Intro = nullptr;

	/**
	 * @brief 당기기 애니메이션 (에디터에서 이름 'Anim_Shift'로 생성 필수).
	 */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_Shift = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 데이터
public:
	/** @brief 슬롯 클릭 알림 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Event")
	FOnSummonSlotClicked OnSlotClicked;

private:
	/** @brief 이 슬롯의 고유 인덱스 (0~4) */
	int32 SlotIndex = -1;

	/** @brief 지연 등장 시 사용할 임시 아이콘 데이터 (캡슐화) */
	UPROPERTY()
	TObjectPtr<UTexture2D> PendingIcon = nullptr;

	/** @brief 지연 등장 시 사용할 임시 코스트 데이터 (캡슐화) */
	int32 PendingCost = 0;

	/** @brief 지연 등장을 제어하는 엔진 타이머 핸들 */
	FTimerHandle RevealTimerHandle;

	///** @brief 최대 쿨타임 변수 */
	//UPROPERTY()
	//float MaxCooldownTime = 0.0f;

	///** @brief 현재 쿨타임 변수 */
	//UPROPERTY()
	//float CurrentCooldownTime = 0.0f;

	///** @brief 쿨타임  */
	//UPROPERTY()
	//FTimerHandle CooldownTimerHandle;
#pragma endregion 데이터
};
