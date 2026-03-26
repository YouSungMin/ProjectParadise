// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/Ingame/Popup/GameResultWidgetBase.h"
#include "DefeatPopupWidget.generated.h"

/**
 * @class UDefeatPopupWidget
 * @brief 게임 패배 시 표시되는 팝업 위젯.
 * @details
 * 1. 기본적으로 부모 클래스의 Home/Retry 기능을 상속받아 사용.
 * 2. 추후 패배 원인 분석 텍스트나 팁 기능 확장을 위해 클래스 분리.
 */
UCLASS()
class PARADISE_API UDefeatPopupWidget : public UGameResultWidgetBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

};
