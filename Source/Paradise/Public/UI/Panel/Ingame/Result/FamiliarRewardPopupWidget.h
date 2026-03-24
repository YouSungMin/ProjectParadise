// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FamiliarRewardPopupWidget.generated.h"

#pragma region 전방 선언
class UImage;
class UTexture2D;
#pragma endregion 전방 선언

/**
 * @class UFamiliarRewardPopupWidget
 * @brief 초회 3별 클리어 시 지급되는 퍼밀리어 보상 연출 위젯
 * @details 승리 팝업과 함께 애니메이션으로 등장하며, 중복 클리어 시에는 표시되지 않습니다.
 */
UCLASS()
class PARADISE_API UFamiliarRewardPopupWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
    virtual void NativeConstruct() override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
    /**
     * @brief 퍼밀리어 아이콘을 세팅하고 등장 애니메이션을 재생합니다.
     * @param InFamiliarID 지급된 퍼밀리어 ID (None이면 아무것도 하지 않음)
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Result")
    void ShowFamiliarReward(FName InFamiliarID);

    /**
     * @brief 위젯을 즉시 숨깁니다.
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Result")
    void HideReward();
#pragma endregion 외부 인터페이스

#pragma region UI 바인딩
protected:
    /** @brief 퍼밀리어 아이콘 이미지 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Img_FamiliarIcon = nullptr;

    /** @brief 배경 이미지 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Background = nullptr;

    /**
     * @brief 등장 애니메이션 (블루프린트에서 지정)
     * @details WBP_FamiliarRewardPopup 애니메이션 탭에서 만들고 이름을 'Anim_Show'로 설정하세요.
     */
    UPROPERTY(Transient, meta = (BindWidgetAnim))
    TObjectPtr<UWidgetAnimation> Anim_Show = nullptr;
#pragma endregion UI 바인딩
};
