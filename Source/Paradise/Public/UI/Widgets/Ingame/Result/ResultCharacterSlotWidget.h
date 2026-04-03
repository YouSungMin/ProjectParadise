// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResultCharacterSlotWidget.generated.h"

#pragma region 전방 선언
class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;
#pragma endregion 전방 선언

/**
 * @struct FResultCharacterData
 * @brief 결과창에 표시할 단일 캐릭터 데이터 구조체.
 */
USTRUCT(BlueprintType)
struct FResultCharacterData
{
	GENERATED_BODY()

	/** @brief 캐릭터 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "CharacterStats"))
	FText CharacterName;

	/** @brief 캐릭터 초상화 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> PortraitImage = nullptr;

	/** @brief 현재 경험치 비율 (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExpPercent = 0.0f;

	/** @brief 이번 판에 획득한 경험치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GainedExp = 0;
};

/**
 * @class UResultCharacterSlotWidget
 * @brief 승리 화면에서 개별 캐릭터 1명의 정보를 표시하는 위젯.
 */
UCLASS()
class PARADISE_API UResultCharacterSlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 슬롯 데이터를 갱신합니다.
	 * @param InData 표시할 캐릭터 정보
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Result")
	void SetSlotData(const FResultCharacterData& InData);
#pragma endregion 외부 인터페이스

#pragma region UI 바인딩
protected:
	/** @brief 캐릭터 초상화 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Portrait = nullptr;

	/** @brief 캐릭터 이름 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Name = nullptr;

	/** @brief 획득 경험치 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GainedExp = nullptr;

	/** @brief 경험치 게이지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Exp = nullptr;
#pragma endregion UI 바인딩
};