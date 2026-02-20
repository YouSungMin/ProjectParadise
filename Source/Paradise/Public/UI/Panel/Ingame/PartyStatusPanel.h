// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PartyStatusPanel.generated.h"

#pragma region 전방 선언
class UCharacterStatusWidget;
class UAbilitySystemComponent;
#pragma endregion 전방 선언

/**
 * @class UPartyStatusPanel
 * @brief 3개의 CharacterStatusWidget을 캡슐화하여 관리하며, 데이터에 따른 일괄 업데이트를 담당합니다.
 */
UCLASS()
class PARADISE_API UPartyStatusPanel : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

public:
#pragma region 초기화 및 데이터 주입
	/**
	 * @brief 특정 인덱스의 멤버 위젯에 데이터를 설정합니다.
	 * @param Index 멤버 인덱스 (0~2)
	 * @param CharacterID 데이터 테이블에서 조회할 캐릭터 고유 키
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void InitializeMember(int32 Index, FName CharacterID);

	/**
	 * @brief 특정 인덱스의 멤버 위젯에 실제 게임플레이 스탯(ASC)을 연결합니다.
	 * @param Index 멤버 인덱스 (0~2)
	 * @param InASC 대상 캐릭터의 Ability System Component
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void BindMemberASC(int32 Index, UAbilitySystemComponent* InASC);
#pragma endregion 초기화 및 데이터 주입

private:
#pragma region 위젯 바인딩
	/** @brief 첫 번째 파티원 위젯 (WBP_Member_0) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCharacterStatusWidget> WBP_Member_0 = nullptr;

	/** @brief 두 번째 파티원 위젯 (WBP_Member_1) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCharacterStatusWidget> WBP_Member_1 = nullptr;

	/** @brief 세 번째 파티원 위젯 (WBP_Member_2) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCharacterStatusWidget> WBP_Member_2 = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 내부 상태 관리
	/** @brief 루프 처리를 위한 위젯 포인터 배열 캐시 */
	UPROPERTY()
	TArray<TObjectPtr<UCharacterStatusWidget>> MemberWidgets;
#pragma endregion 내부 상태 관리
};
