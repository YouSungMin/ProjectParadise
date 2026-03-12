// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseEnhanceDetailWidget.generated.h"

#pragma region 전방 선언
class UImage;
class UTextBlock;
class UButton;
class USoundBase;
class UWidgetAnimation;
#pragma endregion 전방 선언

// 팝업(부모)에게 클릭 이벤트를 토스하기 위한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnhanceActionClicked);

/** @brief 연출(애니메이션/머티리얼)이 끝났음을 상위 팝업에게 알리는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnhanceAnimationFinished);

/**
 * @class UParadiseEnhanceDetailWidget
 * @brief 강화/돌파 대상의 상세 정보를 표시하는 순수 View 위젯
 * @details 계산 로직 없이 전달받은 데이터만 화면에 렌더링하며, 대상 타입에 따라 버튼을 스위칭합니다.
 */
UCLASS()
class PARADISE_API UParadiseEnhanceDetailWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 외부 인터페이스
public:
	/**
	 * @brief 상세 패널의 UI를 갱신합니다.
	 * @param ItemData UI 표시용 기본 데이터 (아이콘, 이름 등)
	 * @param TabType 대상의 타입 (0:캐릭터, 1:무기, 2:방어구, 3:유닛)
	 * @param Cost 소모 재화량
	 * @param CurrentStat 현재 스탯 텍스트 (예: "공격력 100")
	 * @param NextStat 다음 레벨 스탯 텍스트 (예: "공격력 120")
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void RefreshDetail(const FSquadItemUIData& ItemData, int32 TabType, int32 Cost, const FString& CurrentStat, const FString& NextStat);

	/** @brief 초기 상태(선택된 아이템이 없을 때)로 화면을 비웁니다. */
	void ClearDetail();

	/**
	 * @brief 강화 결과에 따른 연출을 재생합니다.
	 * @param bSuccess 강화 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void PlayEnhancementFX(bool bSuccess);
#pragma endregion 외부 인터페이스

#pragma region 델리게이트 (이벤트 전달용)
public:
	UPROPERTY(BlueprintAssignable) FOnEnhanceActionClicked OnEnhanceClicked;
	UPROPERTY(BlueprintAssignable) FOnEnhanceActionClicked OnBreakthroughClicked;

	UPROPERTY(BlueprintAssignable) FOnEnhanceAnimationFinished OnEnhanceAnimFinished;
#pragma endregion 델리게이트

#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region 내부 이벤트 핸들러
private:
	UFUNCTION() void HandleEnhanceBtn();
	UFUNCTION() void HandleBreakthroughBtn();

	/** @brief UMG 애니메이션 종료 시 호출될 콜백 함수 */
	UFUNCTION() void HandleAnimationFinished();
#pragma endregion 내부 이벤트 핸들러

#pragma region UI 컴포넌트 바인딩
protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Img_ItemIcon = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_ItemName = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_Cost = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_CurrentStat = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_NextStat = nullptr;

	/** @brief 무기/방어구용 강화 버튼 */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Enhance = nullptr;
	/** @brief 캐릭터용 돌파 버튼 */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Breakthrough = nullptr;

	/** @brief 강화 성공 시 재생할 UMG 애니메이션 (머티리얼 Opacity 제어용) */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_SuccessFX = nullptr;
#pragma endregion UI 컴포넌트 바인딩

#pragma region 데이터 드리븐 설정
protected:
	/** @brief 강화/돌파 성공 시 재생할 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Enhancement|FX")
	TObjectPtr<USoundBase> Sound_EnhanceSuccess = nullptr;

	/** @brief 강화/돌파 실패 시 재생할 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Enhancement|FX")
	TObjectPtr<USoundBase> Sound_EnhanceFail = nullptr;
#pragma endregion 데이터 드리븐 설정
};
