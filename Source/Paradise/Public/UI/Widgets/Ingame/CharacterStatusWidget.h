// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"
#include "CharacterStatusWidget.generated.h"

#pragma region 전방 선언
class UImage;
class UProgressBar;
class UTexture2D;
class UTextBlock;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;
#pragma endregion 전방 선언

/**
 * @class UCharacterStatusWidget
 * @brief 캐릭터의 상태 정보를 시각화하며, GAS 어트리뷰트와 실시간으로 연동됩니다.
 * @details 캐릭터의 초상화와 GAS 기반 HP/MP 상태를 관리하는 위젯입니다.
 */
UCLASS()
class PARADISE_API UCharacterStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
#pragma region 외부 인터페이스
    /**
     * @brief 캐릭터의 초상화 이미지를 동적으로 설정합니다. (Data-Driven)
     * @param NewPortrait 적용할 텍스처 데이터 포인터
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
    void SetCharacterPortrait(UTexture2D* NewPortrait);

    /**
     * @brief ASC가 완벽히 초기화된 직후 외부에서 호출하여 UI와 GAS를 연결합니다.
     * @param InASC 대상 캐릭터의 Ability System Component
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|GAS")
    void BindToASC(UAbilitySystemComponent* InASC);
#pragma endregion 외부 인터페이스

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

#pragma region 어트리뷰트 콜백
    /**
     * @brief 체력 변경 시 호출되는 콜백 함수.
     * @param Data 변경된 체력의 상세 정보(이전 값, 현재 값 포함)
     */
    void OnHealthChanged(const FOnAttributeChangeData& Data);
    /**
     * @brief 마나 변경 시 호출되는 콜백 함수.
     * @param Data 변경된 마나의 상세 정보(이전 값, 현재 값 포함)
     */
    void OnManaChanged(const FOnAttributeChangeData& Data);
#pragma endregion 어트리뷰트 콜백

private:
#pragma region 데이터 바인딩
    /** @brief 캐릭터 초상화 이미지 (WBP_Portrait) */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Image_Portrait = nullptr;

    /** @brief 에디터 WBP와 연결될 체력 바 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> PB_HealthBar = nullptr;

    /** @brief 에디터 WBP와 연결될 체력 수치 텍스트 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_Health = nullptr;

    /** @brief 에디터 WBP와 연결될 마나 바 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> PB_ManaBar = nullptr;

    /** @brief 에디터 WBP와 연결될 마나 수치 텍스트 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_Mana = nullptr;
#pragma endregion 데이터 바인딩

#pragma region 내부 데이터 관리
    /** @brief 최적화를 위한 GAS 컴포넌트 약참조 (메모리 누수 방지) */
    TWeakObjectPtr<UAbilitySystemComponent> CachedASC = nullptr;
#pragma endregion 내부 데이터 관리

};
