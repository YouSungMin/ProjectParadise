// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InGameController.generated.h"


class APlayerBase;
class APlayerData;
class AAIController;
class UAutoCombatComponent;
class USquadControlComponent;
class UUltimateEffectComponent;
class UParadiseGameInstance;
class AInGamePlayerState;
class UInputMappingContext;
class UInputAction;
class UInGameHUDWidget; //[추가] 26/02/04, 담당자 : 최지원 

struct FInputActionValue;

/**
 * @brief 인게임 플레이어 컨트롤러
 * @details 영웅 교체(Switching), 스쿼드 명령, UI 인터랙션을 담당합니다.
 */
UCLASS()
class PARADISE_API AInGameController : public APlayerController
{
	GENERATED_BODY()

public:
	AInGameController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	//  스쿼드 제어 (Squad Control)
public:
#pragma region Getter 함수

	class UAutoCombatComponent* GetAutoCombatComponent() { return AutoCombatComponent; }
	class USquadControlComponent* GetSquadControlComponent() { return SquadControlComponent; }

	/** @brief 궁극기 화면 연출 컴포넌트를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Paradise|Effect")
	class UUltimateEffectComponent* GetUltimateEffectComponent() const { return UltimateEffectComponent; }
#pragma endregion Getter 함수



#pragma region 0226 김성현 - 디버그 치트 함수 추가

	UFUNCTION(Exec)
	void CheatStageClear();

	UFUNCTION(Exec)
	void CheatStageFail();

	UFUNCTION(Exec)
	void CheatKillCharacter(int32 PlayerIndex);

	UFUNCTION(Exec)
	void CheatRespawn(int32 PlayerIndex);
#pragma endregion 0226 김성현 - 디버그 치트 함수 추가


	/** @brief 캐릭터 사망 시 호출되어 다음 생존 캐릭터로 자동 교체합니다. */
	void OnPlayerDied(APlayerBase* DeadPlayer);

#pragma region 화면 연출 컴포넌트
protected:
	/** @brief 궁극기 사용 시 포스트 프로세스 연출을 담당하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Components")
	TObjectPtr<class UUltimateEffectComponent> UltimateEffectComponent = nullptr;
#pragma endregion 화면 연출 컴포넌트

#pragma region UI 제어 (추가, 26/02/04, 담당자 : 최지원)
public:
	

	/**
	 * @brief [단일 책임 원칙(SRP) 핵심] 생성된 캐릭터(데이터)를 UI와 연동합니다.
	 * @param PlayerIndex 파티 내 인덱스 (0~2)
	 * @param InPlayerData 연동할 데이터(영혼) 객체
	 */
	void BindPlayerToUI(int32 PlayerIndex, APlayerData* InPlayerData);


public:
	/**
	 * @brief 생성된 HUD 위젯 인스턴스를 반환합니다.
	 * @details 초기화 타이밍이 꼬여서 HUD가 없다면, 지연 초기화(Lazy Init)를 통해 즉시 생성합니다.
	 * @return UInGameHUDWidget 포인터
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	UInGameHUDWidget* GetOrCreateInGameHUD();

	/** @brief 액션 패널의 활성화 여부를 설정합니다. */
	void SetActionPanelEnabled(bool bEnabled);

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

#pragma region 자동 모드 관련

public:
	/**
	* @brief 자동 전투 모드를 활성화하거나 비활성화합니다.
	* @param bEnable true일 경우 전체 뷰 시점으로 전환하고 AI 로직 실행합니다.
	*/
	UFUNCTION(Exec,BlueprintCallable, Category = "Squad|Control")
	void ToggleAutoBattleMode(bool bEnable);

protected:

	/** @brief 자동 전투 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UAutoCombatComponent> AutoCombatComponent= nullptr;


#pragma endregion 자동 모드 관련

#pragma region 스쿼드 관리 컴포넌트 

	/** @brief 스쿼드 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USquadControlComponent> SquadControlComponent = nullptr;

#pragma endregion 스쿼드 관리 컴포넌트 

protected:

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

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move = nullptr;

	/** @brief 퍼밀리어 소환 액션 키 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Summon")
	TObjectPtr<UInputAction> IA_SummonSlot1 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Summon")
	TObjectPtr<UInputAction> IA_SummonSlot2 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Summon")
	TObjectPtr<UInputAction> IA_SummonSlot3 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Summon")
	TObjectPtr<UInputAction> IA_SummonSlot4 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Summon")
	TObjectPtr<UInputAction>IA_SummonSlot5 = nullptr;

	/** @brief 공격, 스킬, 궁극기 액션 키 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Attack = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Skill = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Ultimate = nullptr;

private:
	/** @brief 퍼밀리어 소환 액션 키와 매칭될 실행 함수 */
	void OnInputSummonSlot1();
	void OnInputSummonSlot2();
	void OnInputSummonSlot3();
	void OnInputSummonSlot4();
	void OnInputSummonSlot5();

	void OnInputMove(const FInputActionValue& Value);

	void OnInputAttack(const FInputActionValue& Value);
	void OnInputSkill(const FInputActionValue& Value);
	void OnInputUltimate(const FInputActionValue& Value);

	/** @brief 소환 공통 처리 함수 */
	void RequestFamiliarSummon(int32 SlotIndex);

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