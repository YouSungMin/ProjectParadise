// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Enums/GameEnums.h"
#include "ActionControlPanel.generated.h"

#pragma region 전방 선언
class UCommonButtonBase;
class USkillSlotWidget;
class APlayerBase;
class AInGameController;
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

public:
#pragma region 외부 인터페이스
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
#pragma endregion 외부 인터페이스
private:
#pragma region 내부 로직
	/** @brief 공격 버튼 클릭 시 발생할 이벤트 핸들러 */
	UFUNCTION()
	void OnAttackButtonClicked();

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
#pragma endregion 내부 로직

private:
#pragma region 위젯 바인딩
	/** @brief 기본 공격 버튼 (Common UI) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> AttackBtn = nullptr;

	/** @brief 액티브 스킬 슬롯 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USkillSlotWidget> SkillSlot_Active = nullptr;

	/** @brief 궁극기 스킬 슬롯 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USkillSlotWidget> SkillSlot_Ultimate = nullptr;

	/** @brief 캐릭터 교체 버튼 리스트 (배열화하여 최적화) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> TagBtn_A = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> TagBtn_B = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> TagBtn_C = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 내부 데이터
	/** @brief 버튼 일괄 처리를 위한 내부 캐싱 배열 */
	UPROPERTY()
	TArray<TObjectPtr<UCommonButtonBase>> TagButtons;

	/** @brief 캐싱된 플레이어 참조 (가비지 컬렉션 및 안전성을 위해 TWeakObjectPtr 사용) */
	TWeakObjectPtr<APlayerBase> CachedPlayer;
#pragma endregion 내부 데이터
};
