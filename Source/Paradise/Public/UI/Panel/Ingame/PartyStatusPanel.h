// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PartyStatusPanel.generated.h"

#pragma region 전방 선언
class UCharacterStatusWidget;
class UAbilitySystemComponent;
class UHorizontalBox;
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
	 * @brief 파티원을 바구니(컨테이너)에 한 명씩 동적으로 추가하고 데이터를 연결합니다.
	 * @details InGameController에서 캐릭터 스폰/편성 시 개별적으로 호출합니다.
	 * @param CharacterID 데이터 테이블에서 조회할 캐릭터 고유 키
	 * @param InASC 대상 캐릭터의 Ability System Component
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void AddPartyMemberUI(FName CharacterID, UAbilitySystemComponent* InASC);

	/**
	 * @brief 파티 재편성 등을 위해 바구니를 비울 때 호출합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void ClearPartyUI();
#pragma endregion 초기화 및 데이터 주입

protected:
	/** * @brief 파티원 상태를 표시할 위젯의 블루프린트 클래스.
	 * @details 하드코딩을 지양하고 기획자가 에디터에서 WBP_CharacterStatus를 할당하여 데이터를 조절할 수 있도록 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Config", meta = (DisplayName = "파티원 상태 위젯 클래스"))
	TSubclassOf<UCharacterStatusWidget> StatusWidgetClass;

private:
#pragma region 위젯 바인딩
	/**
	 * @brief 동적으로 생성된 파티원 위젯들을 담을 가로 박스 컨테이너.
	 * @details UMG 에디터의 [가로 박스]와 연결되며, 여기에 AddChild를 수행합니다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HB_MemberContainer = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 내부 상태 관리
	/** @brief 루프 처리를 위한 위젯 포인터 배열 캐시 */
	UPROPERTY()
	TArray<TObjectPtr<UCharacterStatusWidget>> MemberWidgets;
#pragma endregion 내부 상태 관리
};
