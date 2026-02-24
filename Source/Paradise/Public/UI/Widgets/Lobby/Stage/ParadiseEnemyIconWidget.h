// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseEnemyIconWidget.generated.h"

#pragma region 전방 선언
class UImage;
class UTexture2D;
#pragma endregion 전방 선언

/**
 * @class UParadiseEnemyIconWidget
 * @brief 스테이지 상세 정보창에 표시되는 개별 적(Enemy) 아이콘 뷰 위젯
 * @details SRP 준수: 비즈니스 로직 없이, 전달받은 텍스처 데이터를 이미지 컴포넌트에 그리는 단일 책임만 수행합니다.
 */
UCLASS()
class PARADISE_API UParadiseEnemyIconWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 부모(StageDetail)로부터 전달받은 텍스처 데이터로 아이콘을 세팅합니다.
	 * @param InFaceIcon 데이터 테이블에서 추출한 적의 초상화 텍스처 (Soft Pointer)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void SetupIcon(TSoftObjectPtr<UTexture2D> InFaceIcon);
#pragma endregion 외부 인터페이스

#pragma region UI 컴포넌트 바인딩
protected:
	/** @brief 적 초상화를 표시할 핵심 이미지 컴포넌트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_EnemyIcon = nullptr;
#pragma endregion UI 컴포넌트 바인딩
};
