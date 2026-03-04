// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseCodexMainWidget.generated.h"

#pragma region 전방 선언
class UButton;
class UParadiseSquadInventoryWidget;
class UInventorySystem;
class UParadiseGameInstance;
#pragma endregion 전방 선언

/** @brief 뒤로가기 버튼 클릭 시 부모(LobbyHUD 등)에게 닫기 요청을 전달 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCodexBackRequested);

/**
 * @class UParadiseCodexMainWidget
 * @brief 수집형 도감(Codex)의 메인 UI를 담당하는 위젯
 * @details 좌측 세로 탭을 통해 카테고리를 전환하며, 데이터 테이블 전체 항목과 인벤토리를 대조하여 보유/미보유 상태를 렌더링합니다.
 */
UCLASS()
class PARADISE_API UParadiseCodexMainWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region 내부 로직
private:
	/** @brief 특정 탭으로 전환하고 데이터를 갱신합니다. */
	void SwitchTab(int32 NewTab);

	/** @brief 현재 선택된 탭에 맞춰 데이터 테이블을 긁어와 리스트 뷰에 전달합니다. */
	void RefreshCodexList();

	/** @brief 안전한 인벤토리 시스템 캐싱 반환 */
	UInventorySystem* GetInventorySystem() const;
#pragma endregion 내부 로직

#pragma region 이벤트 핸들러
private:
	UFUNCTION() void OnClickTabChar();
	UFUNCTION() void OnClickTabWeapon();
	UFUNCTION() void OnClickTabArmor();
	UFUNCTION() void OnClickTabUnit();
	UFUNCTION() void OnClickTabMisc();
	UFUNCTION() void OnClickBack();
#pragma endregion 이벤트 핸들러

#pragma region UI 컴포넌트 바인딩
protected:
	/** @brief 우측 상단 뒤로가기 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Back = nullptr;

	// --- 좌측 세로 탭 버튼들 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Character = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Weapon = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Armor = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Unit = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Misc = nullptr;

	/** @brief 우측에 배치될 공용 인벤토리 뷰 위젯 (재활용) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseSquadInventoryWidget> WBP_CodexList = nullptr;
#pragma endregion UI 컴포넌트 바인딩

#pragma region 상태 데이터
private:
	/** @brief 현재 선택된 탭 인덱스 */
	int32 CurrentTabIndex = -1;

	/** @brief 빠른 데이터 참조를 위한 게임 인스턴스 캐싱 */
	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;
#pragma endregion 상태 데이터

#pragma region 델리게이트
public:
	/** @brief 도감 나가기 이벤트 (LobbyHUD 등에서 바인딩하여 처리) */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnCodexBackRequested OnBackRequested;
#pragma endregion 델리게이트
};
