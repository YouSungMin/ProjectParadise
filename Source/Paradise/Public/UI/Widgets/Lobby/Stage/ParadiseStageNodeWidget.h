// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Structs/StageStructs.h"
#include "ParadiseStageNodeWidget.generated.h"

#pragma region 전방 선언
class UImage;
class UTextBlock;
class UButton;
#pragma endregion 전방 선언

// 노드가 클릭되었을 때 ID를 전달할 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageNodeClicked, FName, SelectedStageID);

/**
 * @class UStageNodeWidget
 * @brief 월드맵(Canvas)에 수동으로 배치되는 개별 스테이지 노드.
 * @details 에디터에서 직접 배치
 */
UCLASS()
class PARADISE_API UParadiseStageNodeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

#pragma region 설정 데이터 (Config)
public:
	/** @brief 스테이지 ID (기획자가 에디터에서 입력) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FName StageID = NAME_None;

	/** @brief 부모 위젯(Manager)에게 클릭 이벤트를 알리는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStageNodeClicked OnNodeClicked;
#pragma endregion 설정 데이터 (Config)

#pragma region UI 컴포넌트
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Thumbnail = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StageName = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Enter = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 로직 (Logic)
public:
	/**
	 * @brief 부모(StageSelect)로부터 데이터를 받아 UI를 갱신하는 함수.
	 * @param InStats 기획 데이터 (이름 등)
	 * @param InAssets 아트 데이터 (이미지 등)
	 */
	void SetupNode(const FStageStats& InStats, const FStageAssets& InAssets);

private:
	UFUNCTION()
	void OnClickEnter();
#pragma endregion 로직 (Logic)
};
