// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InGameController.generated.h"


class APlayerBase;
class APlayerData;
class AAIController;
class UParadiseGameInstance;
class AInGamePlayerState;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UInGameHUDWidget; //[추가] 26/02/04, 담당자 : 최지원 

/**
 * @brief 인게임 플레이어 컨트롤러
 * @details 영웅 교체(Switching), 스쿼드 명령, UI 인터랙션을 담당합니다.
 */
UCLASS()
class PARADISE_API AInGameController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	//  스쿼드 제어 (Squad Control)
public:

	/**
	 * @brief OverViewCamera를 찾아서 초기화해두는 함수
	 */
	void InitializeOverviewCamera();

	/**
	 * @brief 자동 전투 모드를 활성화하거나 비활성화합니다.
	 * @param bEnable true일 경우 전체 뷰 시점으로 전환하고 AI 로직을 강화합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad|Control")
	void SetAutoBattleMode(bool bEnable);

	/*
	 * @brief 요청된 인덱스의 영웅으로 직접 조작 대상을 변경(빙의)하는 함수
	 * @details 기존 영웅에는 AI를 다시 심어주고, 새 영웅의 제어권을 가져옵니다.
	 * @param PlayerIndex 교체할 스쿼드 인덱스 (0 ~ 2)
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	void RequestSwitchPlayer(int32 PlayerIndex);

	/** * @brief 특정 인덱스의 스쿼드 멤버를 부활(재소환) 시킵니다.
	 * @param MemberIndex : 부활시킬 멤버의 인덱스 (0~2)
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad|Command")
	void RespawnSquadPlayer(int32 MemberIndex);

	/** @brief 캐릭터 사망 시 호출되어 다음 생존 캐릭터로 자동 교체합니다. */
	void OnPlayerDied(APlayerBase* DeadPlayer);

	/**
	 * @brief 카메라 시점을 현재 상태(전멸 여부, 자동 모드 여부)에 따라 갱신합니다.
	 * @details SetViewTargetWithBlend를 사용하여 부드러운 시점 전환을 처리합니다.
	 */
	void UpdateCameraSystem();

	/*
	 * @brief 게임 시작 시 스쿼드 3명을 월드에 스폰하고 초기화하는 함수
	 * @details PlayerState의 데이터를 기반으로 실제 육체(Pawn)를 생성합니다.
	 */
	void InitializeSquadPawns();


private:
	
	/*
	 * @brief 현재 조종하지 않는 캐릭터에게 AI 컨트롤러를 빙의시키는 함수
	 */
	void PossessAI(APlayerBase* TargetCharacter);

	/**
	 * @brief [단일 책임 원칙(SRP) 핵심] 생성된 캐릭터(데이터)를 UI와 연동합니다.
	 * @param PlayerIndex 파티 내 인덱스 (0~2)
	 * @param InPlayerData 연동할 데이터(영혼) 객체
	 */
	void BindPlayerToUI(int32 PlayerIndex, APlayerData* InPlayerData);

#pragma region UI 제어 (추가, 26/02/04, 담당자 : 최지원)
public:
	/**
	 * @brief 생성된 HUD 위젯 인스턴스를 반환합니다.
	 * @details 초기화 타이밍이 꼬여서 HUD가 없다면, 지연 초기화(Lazy Init)를 통해 즉시 생성합니다.
	 * @return UInGameHUDWidget 포인터
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	UInGameHUDWidget* GetOrCreateInGameHUD();

protected:
	/** 
	 * @brief 생성할 HUD 위젯 클래스 (BP_InGameHUD 할당용)
	 * @details 에디터에서 WBP_InGameHUD를 할당하세요.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI")
	TSubclassOf<UInGameHUDWidget> InGameHUDClass;

	/** @brief 생성된 HUD 위젯 인스턴스 (메모리 관리용 UPROPERTY) */
	UPROPERTY(BlueprintReadOnly, Category = "Paradise|UI")
	TObjectPtr<UInGameHUDWidget> InGameHUDInstance = nullptr;
#pragma endregion UI 제어

	//  입력 핸들러 (Input Handlers)
private:
	/* 숫자키 1, 2, 3 입력 처리 */
	void OnInputSwitchHero1(const FInputActionValue& Value);
	void OnInputSwitchHero2(const FInputActionValue& Value);
	void OnInputSwitchHero3(const FInputActionValue& Value);

#pragma region 내부 헬퍼 함수
private:
	/**
	 * @brief 캐릭터 교체 후 스킬 및 궁극기 UI를 갱신합니다.
	 * @details 준수를 위해 RequestSwitchPlayer에서 UI 데이터 추출 및 갱신 로직
	 * @param PlayerIndex 갱신할 캐릭터의 스쿼드 인덱스
	 */
	void UpdateActionPanelUI(int32 PlayerIndex);
#pragma endregion 내부 헬퍼 함수

protected:
	//  데이터 및 설정 (Data & Config)
	/*
	 * @brief 현재 필드에 나와 있는 내 영웅들의 육체(Pawn) 캐싱
	 * @details Index 0: 1번 영웅, Index 1: 2번 영웅 ...
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Squad")
	TArray<TObjectPtr<APlayerBase>> ActiveSquadPawns;

	/*
	 * @brief 현재 내가 직접 조종 중인 영웅의 인덱스
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	int32 CurrentControlledIndex = 0;

	/*
	 * @brief 동료(AI) 영웅에게 할당할 AI 컨트롤러 클래스
	 */
	UPROPERTY(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        EditDefaultsOnly, Category = "Squad")
	TSubclassOf<AAIController> SquadAIControllerClass = nullptr;

	//  입력 에셋 (Input Assets)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	/* 영웅 교체 액션 (1, 2, 3번 키) */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_SwitchHero1 = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_SwitchHero2 = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_SwitchHero3 = nullptr;

	/* * @brief 테스트용 캐릭터 클래스 (BP_PlayerBase 할당용)
	 * @details 여기에 에디터에서 만든 캐릭터 블루프린트를 넣어주세요.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Test")
	TSubclassOf<APlayerBase> TestPlayerClass = nullptr;

	/** @brief 전장을 조망하는 전체 뷰 전용 카메라 액터 (에디터에서 할당) */
	UPROPERTY(EditAnywhere, Category = "Squad|Camera")
	TObjectPtr<AActor> OverviewCameraActor = nullptr;

	/** @brief 전체 뷰 카메라를 찾기 위한 태그 (기본값: "Camera.Overview") */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Camera")
	FName OverviewCameraTag = TEXT("Camera.Overview");

	/** @brief 현재 자동 전투 모드 활성화 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad|Status")
	bool bIsAutoMode = false;

	/** @brief 모든 영웅이 사망했는지 여부 */
	bool bIsSquadWipedOut = false;

	/** @brief 카메라 전환 시 걸리는 블렌딩 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Camera")
	float CameraBlendTime = 1.5f;

	/** @brief  전멸 직전 마지막 시점 위치 기억용 */
	FVector LastDeathLocation = FVector::ZeroVector;

	/** @brief  전멸 직전 마지막 시점 회전 기억용 */
	FRotator LastDeathRotation = FRotator::ZeroRotator;

private:
	/**
	 * @brief 게임 인스턴스 캐싱
	 * @details BeginPlay에서 1회만 초기화합니다.
	 */
	TWeakObjectPtr<UParadiseGameInstance> CachedGameInstance = nullptr;

	/** 
	 * @brief 플레이어 스테이트 캐싱
	 */
	TWeakObjectPtr<AInGamePlayerState> CachedPlayerState = nullptr;
};