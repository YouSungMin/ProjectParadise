// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "HomeBaseHPWidget.generated.h"

#pragma region 전방 선언
class UProgressBar;
class UTextBlock;
class UAbilitySystemComponent;
class AHomeBase;
#pragma endregion 전방 선언

/**
 * @class UHomeBaseHPWidget
 * @brief 홈베이스 체력바 위젯. GAS 델리게이트로 실시간 갱신합니다.
 */
UCLASS()
class PARADISE_API UHomeBaseHPWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
    virtual void NativeDestruct() override;

#pragma region 외부 인터페이스
public:
    /**
     * @brief 홈베이스 ASC를 연결하여 체력 감지를 시작합니다.
     * @param InHomeBase 연결할 홈베이스
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
    void InitWithHomeBase(AHomeBase* InHomeBase);
#pragma endregion 외부 인터페이스

#pragma region 위젯 바인딩
private:
    /** @brief 체력 프로그레스바 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> PB_HP = nullptr;

    /** @brief 체력 수치 텍스트 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_HP = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 내부 로직
private:
    /** @brief 체력 변화 콜백 */
    void OnHealthChanged(const FOnAttributeChangeData& Data);
    void OnMaxHealthChanged(const FOnAttributeChangeData& Data);

    /** @brief UI 갱신 */
    void UpdateHPBar();

    /** @brief ASC 약참조 (델리게이트 해제용) */
    TWeakObjectPtr<UAbilitySystemComponent> CachedASC = nullptr;

    /** @brief 현재/최대 체력 캐싱 */
    float CurrentHP = 0.0f;
    float MaxHP = 1.0f;
#pragma endregion 내부 로직
};
