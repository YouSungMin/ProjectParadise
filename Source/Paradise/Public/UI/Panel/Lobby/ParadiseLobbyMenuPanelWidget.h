// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Enums/ParadiseLobbyEnums.h"
#include "ParadiseLobbyMenuPanelWidget.generated.h"

#pragma region 전방 선언
class UButton;
class ALobbyPlayerController;
#pragma endregion 전방 선언

/**
 * @class UParadiseLobbyMenuPanelWidget
 * @brief 로비 중앙 우측에 배치되는 메인 메뉴 버튼 그룹 위젯.
 * @details 각 버튼 클릭 시 컨트롤러에게 메뉴 변경 요청을 보냅니다.
 */
UCLASS()
class PARADISE_API UParadiseLobbyMenuPanelWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

#pragma region UI 컴포넌트
protected:
	/** @brief 전투(스테이지) 입장 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Battle = nullptr;

	/** @brief 소환(가챠) 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Summon = nullptr;

	/** @brief 편성(스쿼드/인벤토리) 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Squad = nullptr;

	/** @brief 강화(성장) 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Enhance = nullptr;

	/** @brief 도감 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Codex = nullptr;
#pragma endregion UI 컴포넌트

#pragma region 내부 로직
private:
	/** @brief 컨트롤러 캐싱 (매번 Cast하지 않기 위함) */
	UPROPERTY()
	TObjectPtr<ALobbyPlayerController> CachedController = nullptr;

	/** @brief 버튼 클릭 핸들러 (공용 혹은 개별) */
	UFUNCTION() void OnClickBattle();
	UFUNCTION() void OnClickSummon();
	UFUNCTION() void OnClickSquad();
	UFUNCTION() void OnClickEnhance();
	UFUNCTION() void OnClickCodex();

	/** @brief 실제 메뉴 변경 요청을 보내는 헬퍼 함수 */
	void RequestMenuChange(EParadiseLobbyMenu InMenu);
#pragma endregion 내부 로직
};
