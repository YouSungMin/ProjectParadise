// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "ParadiseCommonButton.generated.h"

#pragma region 전방 선언
class UTextBlock;
class UImage;
class UTexture2D;
#pragma endregion 전방 선언


/**
 * @brief 텍스트 라벨 기능을 포함한 커스텀 Common Button 
 */
UCLASS()
class PARADISE_API UParadiseCommonButton : public UCommonButtonBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

#pragma region 상태 변화 오버라이드 (Common UI)
	/** @brief 버튼이 눌렸을 때 호출 */
	virtual void NativeOnPressed() override;
	/** @brief 버튼에서 손을 뗐을 때 호출 */
	virtual void NativeOnReleased() override;
	/** @brief 마우스가 올라갔을 때 호출 */
	virtual void NativeOnHovered() override;
	/** @brief 마우스가 벗어났을 때 호출 */
	virtual void NativeOnUnhovered() override;
#pragma endregion 상태 변화 오버라이드 (Common UI)

public:
#pragma region 외부 인터페이스
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void SetButtonText(FText InText);

	/**
	 * @brief 런타임에 버튼 이미지를 동적으로 교체합니다. (스킬 아이콘, 오토 버튼, 설정 등)
	 * @details PressedIcon을 nullptr로 넘기면 NormalIcon과 동일하게 자동 설정됩니다.
	 * @param InNormalIcon 평상시(Normal) 보여줄 기본 이미지
	 * @param InPressedIcon 눌렸을 때(Pressed) 보여줄 이미지 (선택적)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void SetButtonIcon(UTexture2D* InNormalIcon, UTexture2D* InPressedIcon = nullptr);

	/**
	 * @brief 태그 버튼의 활성/비활성 시각 상태를 설정합니다.
	 * @param bIsActive true = 현재 조작 중(밝게), false = 교체 가능(어둡게)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void SetTagActiveState(bool bIsActive);

	/**
	 * @brief 빙글빙글 도는 빛 링 이펙트를 개별적으로 켜거나 끕니다. (오토 모드 등)
	 * @param bIsActive true면 링 활성화, false면 비활성화
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void SetGlowRingActive(bool bIsActive);
#pragma endregion 외부 인터페이스

private:
#pragma region 내부 헬퍼 로직
	/** @brief 중복되는 UI 갱신 로직을 하나로 통합한 함수 */
	void UpdateBackgroundImage(UTexture2D* TargetTex);

	/** @brief 상태 무관 고정 아이콘 교체 헬퍼 (Img_Icon 전담) */
	void UpdateIconImage(UTexture2D* TargetTex);
#pragma endregion 내부 헬퍼 로직

protected:
#pragma region 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI")
	FText ButtonLabelText;

	/** @brief 평상시(Normal) 배경 이미지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI|Style")
	TObjectPtr<UTexture2D> BgImage_Normal = nullptr;

	/** @brief 눌렸을 때(Pressed) 배경 이미지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI|Style")
	TObjectPtr<UTexture2D> BgImage_Pressed = nullptr;

	/** @brief 현재 조작 중인 태그 버튼의 틴트 색상 (기획자 조절 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI|TagButton",
		meta = (DisplayName = "태그 활성(조작 중) 색상"))
	FLinearColor TagActiveColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	/** @brief 교체 대기 중인 태그 버튼의 틴트 색상 (기획자 조절 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI|TagButton",
		meta = (DisplayName = "태그 비활성(대기) 색상"))
	FLinearColor TagInactiveColor = FLinearColor(0.4f, 0.4f, 0.4f, 0.7f);
#pragma endregion 데이터

private:
#pragma region 위젯 바인딩
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Label = nullptr;

	/** @brief 버튼 상태(Normal/Pressed)에 따라 교체될 배경 이미지 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Bg = nullptr;

	/**
	 * @brief 상태 변화와 무관하게 항상 표시될 아이콘 이미지 (FaceIcon 등)
	 * @details Img_Bg가 상태별 배경을 담당하고, Img_Icon은 항상 위에 고정됩니다.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon = nullptr;

	/**
	 * @brief 버튼 테두리를 따라 도는 강조 링 이미지
	 * @details 에디터에서 WBP 셋업 시 바인딩되며, SetGlowRingActive()로 제어됩니다.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_GlowRing = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 데이터 드리븐 설정
protected:
	/** @brief true일 때만 눌림 틴트 효과를 적용합니다. 공격 버튼 등 피드백이 필요한 버튼에만 활성화하세요. */
	UPROPERTY(EditAnywhere, Category = "Paradise|UI|Button")
	bool bEnablePressedTint = false;

	/** @brief 버튼 눌렸을 때 아이콘 틴트 (어둡게) */
	UPROPERTY(EditAnywhere, Category = "Paradise|UI|Button")
	FLinearColor PressedTintColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	/** @brief 버튼 기본 상태 아이콘 틴트 */
	UPROPERTY(EditAnywhere, Category = "Paradise|UI|Button")
	FLinearColor NormalTintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
#pragma endregion 데이터 드리븐 설정
};
