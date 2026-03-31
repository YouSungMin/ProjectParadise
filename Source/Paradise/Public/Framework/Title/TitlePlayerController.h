// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePlayerController.generated.h"

/**
 * @class ATitlePlayerController
 * @brief 타이틀 화면 전용 플레이어 컨트롤러.
 * @details 게임 시작 시 WBP_TitleHUD를 띄우고, 입력 모드를 UI Only로 설정하여 캐릭터 조작을 막고 터치/클릭을 활성화합니다.
 */
UCLASS()
class PARADISE_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

#pragma region 설정
protected:
	/** @brief 타이틀 화면에 띄울 위젯 클래스 (WBP_TitleHUD 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI")
	TSubclassOf<UUserWidget> TitleHUDClass;
#pragma endregion 설정

#pragma region 커서 설정
protected:
    /**
     * @brief 커스텀 커서 위젯 클래스
     * @details 에디터에서 WBP_ParadiseCursor를 할당하세요.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Cursor")
    TSubclassOf<class UParadiseCursorWidget> CursorWidgetClass;

    /**
     * @brief 커스텀 커서 텍스처
     * @details 에디터에서 원하는 커서 이미지를 할당하세요.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Cursor")
    TObjectPtr<UTexture2D> Tex_CustomCursor = nullptr;
#pragma endregion 커서 설정
};
