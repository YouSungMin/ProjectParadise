// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillSlotWidget.generated.h"

#pragma region 전방 선언
class UParadiseCommonButton;
class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;
#pragma endregion 전방 선언

// 쿨타임이 없을 때 스킬 사용을 부모에게 요청하는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillActionRequested);

/**
 * @class USkillSlotWidget
 * @brief 개별 스킬의 아이콘 표시 및 쿨타임 오버레이 로직을 전담합니다. 캐릭터 태그 시 상위 패널에 의해 데이터가 재설정됩니다.
 */
UCLASS()
class PARADISE_API USkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	USkillSlotWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
#pragma region 데이터 업데이트
	/**
	 * @brief 스킬 슬롯의 정보를 갱신합니다.
	 * @param InIconTexture 교체할 스킬 아이콘 텍스처입니다.
	 * @param InMaxCooldownTime 해당 스킬의 최대 쿨타임 정보입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void UpdateSlotInfo(UTexture2D* InIconTexture, float InMaxCooldownTime);

	/**
	 * @brief 쿨타임 애니메이션을 시작하거나 갱신합니다.
	 * @param CurrentTime 현재 남은 쿨타임입니다.
	 * @param MaxTime 전체 쿨타임입니다. (UpdateSlotInfo에서 설정된 값을 덮어쓸 경우 사용)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void RefreshCooldown(float CurrentTime, float MaxTime);
#pragma endregion 데이터 업데이트

#pragma region Getter
	/**
	 * @brief 내부 버튼에 접근하기 위한 Getter입니다.
	 * @return 버튼 위젯의 포인터를 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	UParadiseCommonButton* GetSlotButton() const { return Btn_SkillAction; }
#pragma endregion Getter

private:
#pragma region 내부 로직 (최적화)
	/** @brief 버튼 클릭 시 호출될 델리게이트 바인딩 함수입니다. */
	void OnSkillButtonClicked();

	/** @brief 타이머에 의해 주기적으로 호출되어 쿨타임 UI를 갱신합니다. */
	void UpdateCooldownVisual();

	/** @brief 쿨타임 UI를 비활성화하고 초기 상태로 되돌립니다. */
	void ClearCooldownVisual();

	/** @brief 버튼 눌림/뗌 처리 */
	UFUNCTION()
	void OnSkillButtonPressed();

	UFUNCTION()
	void OnSkillButtonReleased();
#pragma endregion 내부 로직 (최적화)

public:
	/** @brief 스킬 사용 조건이 충족되었을 때 발생 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnSkillActionRequested OnSkillActionRequested;

private:
#pragma region 쿨타임 설정
	/** 
	 * @brief 쿨타임 UI 업데이트 주기 (초)입니다.
	 * @details 값이 작을수록 숫자가 부드럽게 갱신되지만 연산 비용이 증가합니다.
	 */
	UPROPERTY(EditAnywhere, Category = "Paradise|UI|Skill", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float UpdateInterval = 0.05f;
#pragma endregion 쿨타임 설정

#pragma region 위젯 바인딩
	/** @brief 스킬 클릭을 담당하는 Common UI 버튼입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> Btn_SkillAction = nullptr;

	/** @brief 스킬 아이콘을 표시하는 이미지입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SkillIcon = nullptr;

	/** @brief 쿨타임 진행 상황을 표시하는 프로그레스 바입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Cooldown = nullptr;

	/** @brief 남은 쿨타임 시간을 숫자로 표시하는 텍스트입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CooldownTime = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 캡슐화 데이터
	/** @brief 현재 스킬의 최대 쿨타임 (비율 계산용) */
	float MaxCooldown = 0.f;

	/** @brief 현재 남은 쿨타임 시간 */
	float CurrentCooldown = 0.f;

	/** @brief 쿨타임 갱신용 타이머 핸들 */
	FTimerHandle CooldownTimerHandle;
#pragma endregion 캡슐화 데이터

#pragma region 데이터 드리븐 설정
protected:
	/**
	 * @brief 스킬 버튼 기본 아이콘 (폴백 이미지)
	 * @details 데이터 테이블에 스킬 아이콘이 연동되기 전까지 표시됩니다.
	 *          WBP_SkillSlotWidget 디테일 패널 Paradise|UI|Skill 카테고리에서 할당해주세요.
	 */
	UPROPERTY(EditAnywhere, Category = "Paradise|UI|Skill")
	TObjectPtr<UTexture2D> Tex_DefaultSkillIcon = nullptr;

	/** @brief 버튼 눌렸을 때 아이콘 틴트 */
	UPROPERTY(EditAnywhere, Category = "Paradise|UI|Skill")
	FLinearColor PressedTintColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	/** @brief 버튼 기본 상태 아이콘 틴트 */
	UPROPERTY(EditAnywhere, Category = "Paradise|UI|Skill")
	FLinearColor NormalTintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
#pragma endregion 데이터 드리븐 설정
};
