// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePlayerController.generated.h"


#pragma region 전방 선언
class UInputMappingContext;
class UInputAction;
class UParadiseTitleHUDWidget;
struct FInputActionValue;
#pragma endregion 전방 선언
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

    virtual void SetupInputComponent() override;

#pragma region 설정
protected:
	/** @brief 타이틀 화면에 띄울 위젯 클래스 (WBP_TitleHUD 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI")
	TSubclassOf<UUserWidget> TitleHUDClass;
#pragma endregion 설정

#pragma region 입력 에셋
protected:
	/** @brief 매핑 컨텍스트 (IMC_Default 등) */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	/** @brief 설정 창 열기 액션 (ESC) */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Input")
	TObjectPtr<UInputAction> IA_OpenSettings = nullptr;
#pragma endregion 입력 에셋

#pragma region 내부 로직
private:
	/** @brief 생성된 타이틀 HUD 위젯 캐싱 */
	UPROPERTY()
	TObjectPtr<UParadiseTitleHUDWidget> CachedTitleHUD = nullptr;

	/** @brief 설정창 토글 중복 방지 플래그 */
	bool bIsTogglingSettings = false;

	/** @brief ESC 키 입력 처리 함수 */
	void OnInputOpenSettings(const FInputActionValue& Value);
#pragma endregion 내부 로직
};
