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

public:
#pragma region 외부 인터페이스
	/**
	 * @brief 버튼의 텍스트 라벨을 변경합니다.
	 * @param InText 표시할 텍스트 내용
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void SetButtonText(FText InText);

	/**
	 * @brief 버튼의 아이콘 이미지를 변경합니다.
	 * @param InIcon 표시할 텍스처 데이터
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void SetButtonIcon(UTexture2D* InIcon);
#pragma endregion 외부 인터페이스

protected:
#pragma region 데이터
	/** @brief 에디터 배치 시 인스턴스별로 덮어쓸 텍스트 내용입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI")
	FText ButtonLabelText;

	/** @brief 에디터 배치 시 인스턴스별로 덮어쓸 아이콘 이미지입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|UI")
	TObjectPtr<UTexture2D> ButtonIcon = nullptr;
#pragma endregion 데이터

private:
#pragma region 위젯 바인딩
	/** @brief 버튼 내부의 텍스트 위젯 (WBP에서 이름이 Text_Label이어야 함) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Label = nullptr;

	/** @brief 버튼 내부의 아이콘 이미지 (WBP에서 이름이 Img_Icon이어야 함) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon = nullptr;
#pragma endregion 위젯 바인딩
};
