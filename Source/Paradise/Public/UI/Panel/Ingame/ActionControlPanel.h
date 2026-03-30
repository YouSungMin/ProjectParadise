// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Enums/GameEnums.h"
#include "ActionControlPanel.generated.h"

#pragma region 전방 선언
class UParadiseCommonButton;
class USkillSlotWidget;
class UTextBlock;
class UProgressBar;
class APlayerBase;
class AInGameController;
class UTexture2D;
class AInGamePlayerState;
class APlayerData;
class UAutoCombatComponent;
struct FOnAttributeChangeData;
#pragma endregion 전방 선언

/**
 * @class UActionControlPanel
 * @brief 우측 하단 전투 컨트롤러 레이아웃입니다. 개별 슬롯 및 버튼의 상태를 데이터에 따라 관리합니다.
 */
UCLASS()
class PARADISE_API UActionControlPanel : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
#pragma region 외부 인터페이스
	/**
	 * @brief 특정 플레이어 인덱스의 데이터를 기반으로 액션 패널 전체를 스스로 갱신합니다.
	 * @details 컨트롤러에 있던 UpdateActionPanelUI 로직을 이관하여 SRP를 준수합니다.
	 * @param PlayerIndex 갱신할 캐릭터의 스쿼드 인덱스
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void RefreshActionPanel(int32 PlayerIndex);

	/**
	 * @brief 데이터 테이블을 기반으로 패널 전체를 초기화합니다.
	 * @param WeaponActionID 무기 스킬 ID
	 * @param UltimateActionID 궁극기 ID
	 * @param AttackIcon 무기별 기본 공격 아이콘 (추가)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void InitActionPanel(FDataTableRowHandle WeaponAttackHandle,FDataTableRowHandle WeaponSkillHandle, FDataTableRowHandle UltimateActionHandle, UTexture2D* AttackIcon = nullptr);

	/**
	 * @brief 로비 편성 데이터(SquadSubsystem)를 읽어와 태그 버튼의 얼굴 이미지를 세팅합니다.
	 * @details 버튼 초기화 시 호출되며, 편성되지 않은 슬롯은 UI에서 숨깁니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void InitTagButtons();

	/**
	 * @brief 특정 스킬 슬롯의 쿨타임 데이터를 갱신합니다. (Optimization: 가상함수 방지)
	 * @param SkillIndex 0: 일반 기술, 1: 필살기
	 * @param CurrentTime 현재 남은 시간
	 * @param MaxTime 전체 쿨타임 시간
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void UpdateSkillCooldown(int32 SkillIndex, float CurrentTime, float MaxTime);

	/**
	 * @brief 현재 선택된 캐릭터 인덱스에 따라 태그 버튼의 활성화 상태를 업데이트합니다.
	 * @param ActiveCharIndex 현재 플레이 중인 캐릭터 번호 (0, 1, 2)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void UpdateTagButtons(int32 ActiveCharIndex);

	/** @brief 외부(컨트롤러나 HUD)에서 플레이어를 직접 꽂아주는 함수 */ 
	void SetOwningPlayerBase(APlayerBase* InPlayer);

	/**
	 * @brief 특정 어빌리티 실행 시, 다른 액션 버튼들의 터치를 차단하거나 해제합니다.
	 * @param bLocked 잠금 여부 (true면 차단)
	 * @param ExecutingActionType 현재 실행 중인 어빌리티 타입 (이 버튼은 차단 제외)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void LockOtherActionButtons(bool bLocked, ECombatActionType ExecutingActionType);

	/**
	 * @brief 궁극기 연출 중 태그 버튼 조작을 막거나 풉니다.
	 * @param bIsEnabled true면 교체 가능, false면 터치 무시
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void SetTagButtonsEnabled(bool bShouldEnable);

	/**
	 * @brief 키보드 모드 진입 시 공격/스킬 버튼들의 단축키 텍스트 노출 여부를 일괄 제어합니다.
	 * @param bShow true면 텍스트 노출, false면 숨김
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void ToggleShortcutKeys(bool bShow);
#pragma endregion 외부 인터페이스

public:
#pragma region 키보드 입력 연동 (InGameController에서 호출)
	/** @brief 키보드 공격 입력 */
	void KeyboardAttack();
	/** @brief 키보드 스킬 입력 */
	void KeyboardSkill();
	/** @brief 키보드 궁극기 입력 */
	void KeyboardUltimate();
#pragma endregion 키보드 입력 연동

private:
#pragma region 내부 로직

	//0317 김성현 - 어빌리티 사거리 및 사용 취소등의 기능 구현을 위한 로직 변경
	UFUNCTION()
	void OnAttackButtonPressed();
	UFUNCTION()
	void OnAttackButtonReleased();

	UFUNCTION()
	void OnActiveSkillPressed();
	UFUNCTION()
	void OnActiveSkillReleased();

	UFUNCTION()
	void OnUltimateSkillPressed();
	UFUNCTION()
	void OnUltimateSkillReleased();

	/**
	 * @brief UI 버튼 입력을 통합하여 플레이어의 ASC로 전달하는 중앙 제어 함수입니다.
	 * @details 하드코딩된 개별 콜백 함수들을 대체하며, 입력 ID에 따라 적절한 어빌리티 신호를 송신합니다.
	 * @param InputID 어빌리티 시스템(GAS)과 매핑된 입력 식별자 (Attack, Skill, Ultimate 등)
	 */
	UFUNCTION()
	void ProcessAbilityInput(EInputID InputID);

	/**
	 * @brief 태그 버튼 클릭 시 컨트롤러에 캐릭터 교체를 요청하는 핸들러입니다.
	 * @details 델리게이트 페이로드(Payload)를 통해 인덱스를 전달받으므로 UFUNCTION을 붙이지 않습니다.
	 * @param CharacterIndex 교체할 대상 캐릭터의 스쿼드 인덱스 (0, 1, 2)
	 */
	void OnTagButtonClicked(int32 CharacterIndex);

	/**
	 * @brief 오토 모드 변경 방송을 수신하여 UI 상호작용을 제어합니다.
	 * @param bIsAuto 오토 전투 켜짐 여부
	 */
	UFUNCTION()
	void HandleAutoBattleStateChanged(bool bIsAuto);

	/** @brief 태그 버튼 쿨타임 애니메이션 갱신 콜백 */
	void UpdateTagCooldownVisual();

	/** @brief 태그 버튼 쿨타임 리셋 및 숨김 처리 */
	void ClearTagCooldownVisual();
#pragma endregion 내부 로직

private:
#pragma region 위젯 바인딩
	/** @brief 기본 공격 버튼 (Common UI) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> AttackBtn = nullptr;

	/** @brief 액티브 스킬 슬롯 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USkillSlotWidget> SkillSlot_Active = nullptr;

	/** @brief 궁극기 스킬 슬롯 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USkillSlotWidget> SkillSlot_Ultimate = nullptr;

	/** @brief 캐릭터 교체 버튼 리스트 (배열화하여 최적화) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> TagBtn_A = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> TagBtn_B = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> TagBtn_C = nullptr;

	/** @brief 궁극기 버튼 누를시에 나올 태그 쿨타임 PB */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_TagCooldown_A = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_TagCooldown_B = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_TagCooldown_C = nullptr;

	/** @brief 쿨타임 숫자를 표시할 텍스트 3개 추가 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TagCooldown_A = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TagCooldown_B = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TagCooldown_C = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 내부 데이터
	/** @brief 버튼 일괄 처리를 위한 내부 캐싱 배열 */
	UPROPERTY()
	TArray<TObjectPtr<UParadiseCommonButton>> TagButtons;

	/** @brief 궁극기 연출 후 태그 잠금을 풀기 위한 타이머 핸들 */
	FTimerHandle TimerHandle_TagLock;

	/**
	 * @brief 게이지 갱신 타이머 핸들
	 * @details 지역 변수 대신 멤버 변수로 선언해야 ClearTimer로 멈출 수 있습니다!
	 */
	FTimerHandle TimerHandle_TagVisual;

	/** @brief 캐싱된 플레이어 참조 (가비지 컬렉션 및 안전성을 위해 TWeakObjectPtr 사용) */
	TWeakObjectPtr<APlayerBase> CachedPlayer = nullptr;

	/** @brief 현재 조작 중인 캐릭터 인덱스 (중복 클릭 방지용) */
	int32 CurrentActiveTagIndex = 0;

	/**
	 * @brief 공격 버튼 기본 아이콘 (폴백 이미지)
	 * @details CharacterAssets에 WeaponIcon이 없거나 로드 실패 시 표시됩니다.
	 *          에디터의 WBP_ActionControlPanel 디테일 패널에서 할당해주세요.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|ActionPanel")
	TObjectPtr<UTexture2D> Tex_DefaultAttackIcon = nullptr;

	/** @brief 캐릭터별 무기 스킬 쿨타임 종료 시각을 기록 (교체 복구용) */
	UPROPERTY()
	TMap<FName, float> ActiveSkillEndTimes;

	/** @brief 캐릭터별 궁극기 쿨타임 종료 시각을 기록 (교체 복구용) */
	UPROPERTY()
	TMap<FName, float> UltimateEndTimes;

	/** @brief 프로그레스 바 일괄 처리를 위한 캐싱 배열 */
	UPROPERTY()
	TArray<TObjectPtr<UProgressBar>> TagCooldownBars;

	/** @brief 텍스트 일괄 제어를 위한 캐싱 배열 */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> TagCooldownTexts;

	/** @brief 현재 태그 쿨타임 */
	float CurrentTagCooldown = 0.0f;
	/** @brief 최대 태그 쿨타임 */
	float MaxTagCooldown = 0.0f;
#pragma endregion 내부 데이터

#pragma region 데이터 드리븐 설정
protected:
	/** @brief 버튼 눌렸을 때 아이콘 틴트 (어둡게) */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Button")
	FLinearColor PressedTintColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	/** @brief 버튼 기본 상태 아이콘 틴트 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Button")
	FLinearColor NormalTintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
#pragma endregion 데이터 드리븐 설정

#pragma region 코스트 연동 로직
private:
	/** @brief 스킬 사용 시 필요한 마나량 (데이터 테이블 캐싱) */
	float CachedActiveManaCost = 0.0f;

	/** @brief 궁극기 사용 시 필요한 마나량 (데이터 테이블 캐싱) */
	float CachedUltimateManaCost = 0.0f;

	/** @brief 현재 조작 중인 캐릭터의 ASC (델리게이트 안전 해제용) */
	TWeakObjectPtr<class UAbilitySystemComponent> CachedASC = nullptr;

	/** @brief 마나 속성이 변경될 때 호출되는 콜백 함수 */
	void OnManaChanged(const FOnAttributeChangeData& Data);

	/**
	 * @brief 현재 마나량과 스킬의 요구 마나량을 비교하여 UI를 갱신합니다.
	 * @param CurrentMana 현재 마나
	 */
	void UpdateSkillUsabilityByMana(float CurrentMana);
#pragma endregion 코스트 연동 로직

#pragma region 사거리 표시용 데이터

	/** @brief 현재 활성화된 무기 기본 공격 핸들 캐싱 (인디케이터 표시용) */
	UPROPERTY()
	FDataTableRowHandle CachedWeaponAttackActionHandle;

	/** @brief 현재 활성화된 무기 스킬 핸들 캐싱 (인디케이터 표시용) */
	UPROPERTY()
	FDataTableRowHandle CachedWeaponSkillActionHandle;

	/** @brief 현재 활성화된 궁극기 스킬 핸들 캐싱 (인디케이터 표시용) */
	UPROPERTY()
	FDataTableRowHandle CachedUltimateActionHandle;

#pragma region 사거리 표시용 데이터
};
