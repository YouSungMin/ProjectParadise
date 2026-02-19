// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageTextWidget.generated.h"

#pragma region 전방 선언
class UTextBlock;
#pragma endregion 전방 선언

/**
 * @class UDamageTextWidget
 * @brief 데미지 수치를 화면에 표시하는 UMG 위젯 클래스입니다.
 * @details 일반/크리티컬 데미지를 색상과 크기로 구분하여 표시하며,
 *          블루프린트에서 구현된 팝업 애니메이션을 실행합니다.
 */
UCLASS()
class PARADISE_API UDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 외부 인터페이스
public:
	/**
	 * @brief 전달받은 데미지 수치를 텍스트 블록에 적용하고 크리티컬 여부에 따라 스타일을 변경합니다.
	 * @param DamageAmount 표시할 데미지 수치
	 * @param bIsCritical 크리티컬 히트 여부 (true: 빨강/크게, false: 흰색/보통)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|DamageText")
	void SetDamageText(float DamageAmount, bool bIsCritical = false);

	/**
	 * @brief 데미지 텍스트가 팝업되는 애니메이션을 C++에서 직접 실행합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|DamageText")
	void PlayPopupAnimation();

	/**
	 * @brief 위젯을 초기 상태로 되돌립니다.
	 * @details 풀 반납 전 호출되어 텍스트와 스타일을 초기화합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|DamageText")
	void ResetWidget();
#pragma endregion 외부 인터페이스

#pragma region 데이터 드리븐 설정
protected:
	/**
	 * @brief 일반 데미지 텍스트 색상.
	 * @details 기획자가 BP 디테일 패널에서 조절할 수 있습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageText|Style", meta = (DisplayName = "일반 데미지 색상"))
	FLinearColor NormalDamageColor = FLinearColor::White;

	/**
	 * @brief 크리티컬 데미지 텍스트 색상.
	 * @details 기획자가 BP 디테일 패널에서 조절할 수 있습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageText|Style", meta = (DisplayName = "크리티컬 데미지 색상"))
	FLinearColor CriticalDamageColor = FLinearColor::Red;

	/**
	 * @brief 크리티컬 히트 시 텍스트 크기 배율.
	 * @details 1.0보다 크면 텍스트가 커집니다. (예: 1.5배)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageText|Style", meta = (ClampMin = "1.0", ClampMax = "3.0", DisplayName = "크리티컬 크기 배율"))
	float CriticalScale = 1.5f;
#pragma endregion 데이터 드리븐 설정

#pragma region 바인딩 위젯
protected:
	/** @brief 데미지 숫자 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Damage = nullptr;
#pragma endregion 바인딩 위젯

#pragma region 애니메이션
protected:
	/** @brief 날아오르는 애니메이션 (BP에서 제작) */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_FlyUp = nullptr;
#pragma endregion 애니메이션
};
