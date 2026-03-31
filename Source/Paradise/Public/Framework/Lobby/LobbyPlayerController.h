// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/Enums/ParadiseLobbyEnums.h"
#include "Data/Structs/GachaTypes.h"
#include "LobbyPlayerController.generated.h"

#pragma region 전방 선언
class UParadiseLobbyHUDWidget;
class ACameraActor;
class AParadiseGachaBoxActor;
class UParadiseGachaResultWidget;
class UParadiseStageSelectWidget;
class UParadiseChapterSelectWidget;
class UAudioComponent;
class UParadiseCursorWidget;
class UParadiseCursorSubsystem;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
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
	virtual void SetupInputComponent() override;

#pragma region 입력 에셋 (추가)
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	/** @brief 설정 창 열기 액션 (ESC) */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Input")
	TObjectPtr<UInputAction> IA_OpenSettings = nullptr;
#pragma endregion 입력 에셋 (추가)

#pragma region 0224 김성현 - 디버그테스트 전용 함수 (삭제예정)

public:
	// 디버그 / 치트 명령어 (콘솔창에 입력 가능)
	UFUNCTION(Exec)
	void CheatAddCharacter(FName CharacterID);

	UFUNCTION(Exec)
	void CheatAddFamiliar(FName FamiliarID);

	UFUNCTION(Exec)
	void CheatAddItem(FName ItemID, int32 Count = 1);

	UFUNCTION(Exec)
	void CheatAddExp(FName CharacterID, int32 ExpAmount);

	UFUNCTION(Exec)
	void CheatSetPlayerSlot(int32 SlotIndex, FName CharacterID);

	UFUNCTION(Exec)
	void CheatSetFamiliarSlot(int32 SlotIndex, FName FamiliarID);

	UFUNCTION(Exec)
	void CheatEquipItem(FName CharacterID, FName ItemID);

	UFUNCTION(Exec)
	void CheatAddGold(int32 Amount);

	UFUNCTION(Exec)
	void CheatAddAether(int32 Amount);

	UFUNCTION(Exec)
	void CheatSellItem(FName ItemID, int32 QuantityToSell = 1);

	UFUNCTION(Exec)
	void CheatAwakenCharacter(FName CharacterID);

	UFUNCTION(Exec)
	void CheatEnhanceEquipment(FName ItemID);

	UFUNCTION(Exec)
	void CheatAddAwakeningPiece(FName CharacterID, int32 Count);

	//모든 아이템, 캐릭터, 퍼밀리어, 돈을 얻는 치트
	UFUNCTION(Exec)
	void CheatGrantAll();

#pragma endregion 0224 김성현 - 디버그테스트 전용 함수 (삭제예정)


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

	/** @brief 실제 상자가 떨어지고 뚜껑이 열리는 연출 전용 시네 카메라 (태그: "Cam_GachaAction") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<ACameraActor> Camera_GachaAction = nullptr;

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

	/** @brief 10연차 연출이 끝난 후 띄워줄 결과창 UI 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI")
	TSubclassOf<UParadiseGachaResultWidget> GachaResultWidgetClass;
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
	TObjectPtr<UParadiseLobbyHUDWidget> CachedLobbyHUD = nullptr;

	/** @brief 매번 생성하지 않기 위해 결과창 위젯을 강하게 쥐고 있습니다. (캐싱 및 Object Pooling) */
	UPROPERTY()
	TObjectPtr<UParadiseGachaResultWidget> CachedResultWidget = nullptr;
#pragma endregion 로직 인터페이스

#pragma region 가챠 연출 제어
public:
	/**
	 * @brief 1회 또는 10회 소환 버튼을 눌렀을 때, 본격적인 가챠 연출을 시작합니다.
	 * @param DrawCount 뽑기 횟수 (1 또는 10)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void StartGachaActionSequence(int32 DrawCount);

	/**
	 * @brief 결과창의 '계속' 버튼이 눌렸을 때 호출됩니다.
	 * @details 박스·구슬 정리 → CurrentMenu 초기화 → SummonPopup 복귀
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void ReturnFromGachaToSummon();

	/**
	 * @brief 가챠 박스 연출이 끝나고 결과창을 띄워달라는 델리게이트를 받을 콜백 함수
	 */
	UFUNCTION()
	void OnShowGachaResultScreen(const TArray<FGachaResult>& FinalResults);

private:
	/**
	 * @brief 현재 씬에 스폰된 가챠 박스 약참조
	 * @details 매 뽑기마다 새로 스폰되며, CleanupAndDestroy() 후 nullptr 로 초기화됩니다.
	 */
	TWeakObjectPtr<AParadiseGachaBoxActor> CachedGachaBox = nullptr;
#pragma endregion 가챠 연출 제어

#pragma region 챕터 및 스테이지 제어
public:
	/**
	 * @brief 챕터 슬롯 클릭 시 호출. 카메라를 이동하고 지도를 교체합니다.
	 * @param ChapterID 선택한 챕터 번호
	 * @param MapTexture 교체할 지도 이미지
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Stage")
	void EnterChapterMap(int32 ChapterID, UTexture2D* MapTexture);

	/** @brief 현재 선택된 챕터 반환 (StageSelect 위젯이 켜질 때 참고함) */
	UFUNCTION(BlueprintPure, Category = "Paradise|Stage")
	int32 GetCurrentSelectedChapter() const { return CurrentSelectedChapter; }

protected:
	/** * @brief 카메라 이동이 끝난 후 화면에 띄울 스테이지(노드) 선택 위젯 클래스
	 * @details BP_LobbyPlayerController에서 WBP_StageSelect를 할당합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI")
	TSubclassOf<class UParadiseStageSelectWidget> StageSelectWidgetClass = nullptr;

private:
	/** @brief 현재 선택된 챕터 ID */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Stage", meta = (AllowPrivateAccess = "true"))
	int32 CurrentSelectedChapter = 1;

	/** @brief 3D 지도 배경 액터 캐싱 */
	UPROPERTY(Transient)
	TObjectPtr<class AParadiseMapEnvironmentActor> CachedMapEnvActor = nullptr;

	/** @brief 생성된 스테이지 선택 위젯을 재사용하기 위한 캐싱 (Object Pooling) */
	UPROPERTY(Transient)
	TObjectPtr<class UParadiseStageSelectWidget> CachedStageSelectWidget = nullptr;
#pragma endregion 챕터 및 스테이지 제어

#pragma region 오디오 제어
private:
	/** @brief 카메라 이동 사운드 제어용 컴포넌트 (위젯이 열릴 때 강제 종료하기 위함) */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CameraSwooshAudioComp = nullptr;

public:
	/** @brief 현재 재생 중인 카메라 무빙 사운드가 있다면 즉시 종료합니다. */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	void StopCameraSwoosh();
#pragma endregion 오디오 제어

#pragma region 커서 설정
protected:
	/**
	 * @brief 커스텀 커서 위젯 클래스
	 * @details 에디터에서 WBP_ParadiseCursor를 할당하세요.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Cursor")
	TSubclassOf<UParadiseCursorWidget> CursorWidgetClass;

	/**
	 * @brief 커스텀 커서 텍스처
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Cursor")
	TObjectPtr<UTexture2D> Tex_CustomCursor = nullptr;

private:
		/** @brief 커서 서브시스템 캐싱 */
		TWeakObjectPtr<UParadiseCursorSubsystem> CachedCursorSubsystem = nullptr;
#pragma endregion 커서 설정

#pragma region 내부 로직
private:
	/** @brief ESC 키 입력 처리 함수 */
	void OnInputOpenSettings(const FInputActionValue& Value);

	/** @brief 설정창 토글 중복 방지 플래그 */
	bool bIsTogglingSettings = false;
#pragma endregion 내부 로직
};