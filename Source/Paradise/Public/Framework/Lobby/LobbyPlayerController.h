// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/Enums/ParadiseLobbyEnums.h"
#include "LobbyPlayerController.generated.h"

#pragma region 전방 선언
class UParadiseLobbyHUDWidget;
class ACameraActor;
#pragma endregion 전방 선언

/**
 * @class ALobbyPlayerController
 * @brief 로비 전용 플레이어 컨트롤러
 */
UCLASS()
class PARADISE_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
#pragma region 0224 김성현 - 디버그테스트 전용 함수 (삭제예정)

public:
	// ==========================================
	// 디버그 / 치트 명령어 (콘솔창에 입력 가능)
	// ==========================================
	UFUNCTION(Exec)
	void CheatAddCharacter(FName CharacterID);

	UFUNCTION(Exec)
	void CheatAddFamiliar(FName FamiliarID);

	UFUNCTION(Exec)
	void CheatAddItem(FName ItemID, int32 Count = 1);

#pragma endregion 0224 김성현 - 디버그테스트 전용 함수 (삭제예정)


#pragma region 카메라 설정

#pragma region 카메라 설정
public:
	/**
	 * @brief 카메라 이동을 요청합니다.
	 * @param TargetMenu 이동할 메뉴 위치 (Battle, None 등)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Camera")
	void MoveCameraToMenu(EParadiseLobbyMenu TargetMenu);

protected:
	/** @brief 메인 로비 카메라 액터 (레벨에 배치된 태그로 찾음: "Cam_Main") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<ACameraActor> Camera_Main;

	/** @brief 전투(스테이지) 작전 지도 카메라 액터 (태그: "Cam_Battle") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<ACameraActor> Camera_Battle;

	/** @brief 소환(가챠) 카메라 액터 (태그: "Cam_Summon") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<ACameraActor> Camera_Summon;

	/** @brief 카메라 이동 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Config")
	float CameraBlendTime = 1.5f;

	/** @brief 카메라 이동 곡선 (Ease In/Out) */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Config")
	TEnumAsByte<EViewTargetBlendFunction> CameraBlendFunc = VTBlend_Cubic;

private:
	/** @brief 카메라 이동 완료 후 UI를 띄우기 위한 타이머 핸들 */
	FTimerHandle TimerHandle_CameraBlend;

	/** @brief 이동 완료 후 실행할 작업 */
	void OnCameraMoveFinished(EParadiseLobbyMenu TargetMenu);
#pragma endregion 카메라 설정

#pragma region 설정 (Config)
protected:
	/** * @brief 로비 화면에 띄울 HUD 위젯 클래스.
	 * @details BP_LobbyPlayerController에서 WBP_LobbyHUD를 넣어주세요.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI")
	TSubclassOf<UParadiseLobbyHUDWidget> LobbyHUDClass;
#pragma endregion 설정

#pragma region 로직 인터페이스
public:
	/**
	 * @brief 메뉴 변경을 요청합니다. (UI 버튼 등에서 호출)
	 * @param InNewMenu 변경할 목표 메뉴.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Lobby")
	void SetLobbyMenu(EParadiseLobbyMenu InNewMenu);

	/** @brief HUD 캐싱용 (필요시 구현) */
	void SetLobbyHUD(UParadiseLobbyHUDWidget* InHUD);

	/** @brief 컨텍스트를 유지한 채 직전 메뉴로 돌아갑니다. */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Navigation")
	void RequestBackToPreviousMenu();

private:
	/** @brief 직전 메뉴 상태를 캐싱합니다. */
	EParadiseLobbyMenu PreviousMenu = EParadiseLobbyMenu::None;

	/** @brief 현재 메뉴 상태 저장 */
	EParadiseLobbyMenu CurrentMenu = EParadiseLobbyMenu::None;

	UPROPERTY()
	TObjectPtr<UParadiseLobbyHUDWidget> CachedLobbyHUD;
#pragma endregion 로직 인터페이스
};