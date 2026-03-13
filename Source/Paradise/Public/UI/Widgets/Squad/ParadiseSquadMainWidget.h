// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseSquadMainWidget.generated.h"

#pragma region 전방 선언
class UParadiseSquadInventoryWidget;
class UParadiseSquadFormationWidget;
class UParadiseSquadDetailWidget;
class UButton;
class UWidgetSwitcher;
class UInventorySystem;
class UParadiseGameInstance;
class USquadSubsystem;
class AParadiseSquadSceneManager;
#pragma endregion 전방 선언

/** @brief 편성 화면에서 뒤로가기 요청 시 발생하는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSquadBackRequested);

/**
 * @class UParadiseSquadMainWidget
 * @brief 편성(Squad) 시스템의 메인 컨트롤러 (Mediator Pattern) 위젯
 * @details GameInstance(데이터 테이블)와 InventoryComponent(보유 목록)를 조회합니다.
 * 일반 모드와 장비 교체 모드 간의 상태 전환을 관리합니다.
 * 하위 위젯(Formation, Detail, Inventory) 간의 상호작용을 중재합니다.
 */
UCLASS()
class PARADISE_API UParadiseSquadMainWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
#pragma endregion 생명주기

#pragma region UI 컴포넌트 바인딩
protected:
	/**
	 * @brief [엔드필드 핵심] 전체 화면을 덮는 스위처
	 * @details Index 0: 메인 3D 투명 화면 (사진 1번) / Index 1: 인벤토리 및 상세창 (사진 2~4번)
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_MainScreen = nullptr;

	// --- 하위 패널 (Child Widgets) ---

	/** @brief 우측 인벤토리 리스트 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseSquadInventoryWidget> WBP_InventoryPanel = nullptr;

	/** @brief 좌측 상단 편성(슬롯) 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseSquadFormationWidget> WBP_FormationPanel = nullptr;

	/** @brief 좌측 하단 상세 정보 및 버튼 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseSquadDetailWidget> WBP_DetailPanel = nullptr;

	// --- 탭 버튼 (Tab Buttons) ---

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Character = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Weapon = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Armor = nullptr;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Tab_Unit = nullptr;

	/**
	 * @brief 3D 캐릭터 모델링 위에 덮어씌울 투명 버튼들
	 * @details 유저가 3D 캐릭터를 누른 것처럼 착각하게 만듭니다.
	 */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Hitbox_Main = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Hitbox_Sub1 = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Hitbox_Sub2 = nullptr;

	/** @brief 뒤로가기 버튼 (로비로 복귀 + 편성 자동 저장) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Back = nullptr;
#pragma endregion UI 컴포넌트 바인딩

#pragma region 로직 - 탭 및 상태 제어
private:
	/** @brief 사진 1번(메인 3D 화면)으로 스위처를 되돌리고 상태를 초기화합니다. */
	UFUNCTION()
	void ReturnToOverviewScreen();

	UFUNCTION() void OnClickCharTab();
	UFUNCTION() void OnClickWpnTab();
	UFUNCTION() void OnClickArmTab();
	UFUNCTION() void OnClickUnitTab();

	// 3D 캐릭터 터치용 래퍼 함수 (내부에서 HandleFormationSlotSelected 호출)
	UFUNCTION() void OnTouch3DCharacter_Main();
	UFUNCTION() void OnTouch3DCharacter_Sub1();
	UFUNCTION() void OnTouch3DCharacter_Sub2();
	/** 
	 * @brief 탭을 전환하고 관련 UI를 갱신합니다.
	 * @param NewTab 전환할 탭 인덱스 (SquadTabs 네임스페이스 참조)
	 */
	UFUNCTION()
	void SwitchTab(int32 NewTab);

	/** @brief 현재 상태(CurrentState)에 따라 버튼 활성/비활성 및 UI 잠금을 처리합니다. */
	UFUNCTION()
	void UpdateUIState();

	/** @brief 상세 패널의 버튼 상태(교체 vs 확인 / 취소)를 갱신하는 헬퍼  */ 
	UFUNCTION()
	void UpdateDetailPanelState();
#pragma endregion 로직 - 탭 및 상태 제어

#pragma region 로직 - 데이터 처리
public:
	/** @brief 현재 탭에 맞는 데이터를 수집하여 인벤토리 패널을 갱신합니다. */
	UFUNCTION()
	void RefreshInventoryUI();

	/**
	 * @brief 패널의 상태를 초기화합니다. (뒤로가기 후 재진입 시 호출)
	 * @details 선택된 슬롯 해제, 상세창 닫기, 탭 초기화 등을 수행합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Squad")
	void ResetPanelState();

	/**
	 * @brief 로비에서 편성 버튼을 눌러 진입할 때 매번 호출해야 하는 함수!
	 * @details 패널을 초기화하고, 3D 카메라로 즉시 시점을 전환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Squad")
	void OnEnterSquadUI();
private:
	/**
	 * @brief SquadSubsystem의 현재 편성 데이터를 읽어 FormationWidget 슬롯을 초기화합니다.
	 * @details NativeConstruct 시점과 편성 변경 확정(Confirm) 이후에 호출합니다.
	 *          캐릭터 슬롯(0~2)은 CharacterAssets/Stats 테이블을, 유닛 슬롯(3~7)은
	 *          FamiliarAssets/Stats 테이블을 조회하여 아이콘 및 레벨 정보를 채웁니다.
	 */
	UFUNCTION()
	void InitFormationFromSubsystem();

	/**
	 * @brief SquadSubsystem의 슬롯 변경 델리게이트를 구독합니다.
	 * @details OnPlayerSlotChanged, OnFamiliarSlotChanged 이벤트에 각각 핸들러를 바인딩합니다.
	 */
	UFUNCTION()
	void BindSquadSubsystemDelegates();

	/**
	 * @brief SquadSubsystem 델리게이트 구독을 해제합니다.
	 * @details NativeDestruct에서 호출하여 메모리 누수 및 댕글링 포인터를 방지합니다.
	 */
	UFUNCTION()
	void UnbindSquadSubsystemDelegates();

	/** 
	 * @brief ID와 레벨 정보를 받아 UI 표시용 데이터 구조체로 변환합니다. (Factory Method)
	 * @param ID 대상의 ID (RowName)
	 * @param InLevel 레벨
	 * @param TabType 어떤 종류의 테이블을 검색할지 결정
	 * @param bUseBodyIcon 캐릭터의 경우 BodyIcon(전신)을 사용할지 여부 (기본값: false = FaceIcon)
	 * @return UI 표시용 데이터 구조체 (FSquadItemUIData)
	 */
	UFUNCTION()
	FSquadItemUIData MakeUIData(FName ID, int32 InLevel, int32 TabType, bool bUseBodyIcon = false);
#pragma endregion 로직 - 데이터 처리

#pragma region 로직 - 이벤트 핸들러
private:
	/**
	 * @brief 인벤토리 아이템 클릭 시 호출됩니다.
	 * @details 일반 모드에서는 정보 표시/교체, 장비 모드에서는 장착 로직을 수행합니다.
	 */
	UFUNCTION()
	void HandleInventoryItemClicked(FSquadItemUIData ItemData);

	/** 
	 * @brief 편성 슬롯 클릭 시 호출됩니다.
	 * @details 선택된 슬롯을 강조하고 상세 정보를 표시합니다.
	 */
	UFUNCTION()
	void HandleFormationSlotSelected(int32 SlotIndex);

	/** @brief [장비 교체] 버튼 클릭 시 -> 교체 모드로 진입합니다. */
	UFUNCTION()
	void HandleSwapEquipmentMode();

	/** @brief [취소/완료] 버튼 클릭 시 -> 일반 모드로 복귀합니다. */
	UFUNCTION()
	void HandleCancelEquipMode();

	/** @brief [캐릭터 교체] 버튼 클릭 시 */
	UFUNCTION()
	void HandleSwapCharacterMode();

	/** @brief [확인] 버튼 클릭 시 -> 실제 교체 수행 */
	UFUNCTION()
	void HandleConfirmAction();

	/** @brief [뒤로가기] 버튼 클릭 시 -> 상위 위젯(LobbyHUD)에 신호 전달 */
	UFUNCTION()
	void HandleBackClicked();

	/**
	 * @brief SquadSubsystem::OnPlayerSlotChanged 델리게이트 수신 핸들러입니다.
	 * @details 특정 플레이어 슬롯(0~2)이 변경되었을 때 FormationWidget을 즉시 갱신합니다.
	 * @param SlotIndex 변경된 슬롯 인덱스
	 * @param NewPlayerID 새로 배치된 플레이어 ID
	 */
	UFUNCTION()
	void OnPlayerSlotUpdated(int32 SlotIndex, FName NewPlayerID);

	/**
	 * @brief SquadSubsystem::OnFamiliarSlotChanged 델리게이트 수신 핸들러입니다.
	 * @details 특정 퍼밀리어 슬롯(0~4)이 변경되었을 때 FormationWidget(슬롯 3~7)을 즉시 갱신합니다.
	 * @param SlotIndex 변경된 퍼밀리어 슬롯 인덱스 (0~4 → UI에서는 3~7에 대응)
	 * @param NewFamiliarID 새로 배치된 퍼밀리어 ID
	 */
	UFUNCTION()
	void OnFamiliarSlotUpdated(int32 SlotIndex, FName NewFamiliarID);
#pragma endregion 로직 - 이벤트 핸들러

#pragma region 데이터 소스 (약한 참조)
private:
	//0212 김성현 - 인벤토리 시스템 Getter 함수로 캐싱 로직 대체
	/**
	 * @brief 현재 게임 세션의 전역 인벤토리 서브시스템을 반환합니다.
	 * @return UInventorySystem 포인터
	 */
	UInventorySystem* GetInventorySystem() const;

	/**
	 * @brief SquadSubsystem을 반환합니다. 편성 읽기/쓰기에 사용합니다.
	 * @return USquadSubsystem 포인터 (없으면 nullptr)
	 */
	USquadSubsystem* GetSquadSubsystem() const;

	/** @brief 데이터 테이블 접근용 (순환 참조 방지) */
	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;

	/** @brief 맵에 배치된 3D 스튜디오(디오라마) 매니저. 편성 변경 시 모델링을 갱신합니다. */
	UPROPERTY()
	TObjectPtr<AParadiseSquadSceneManager> SceneManager = nullptr;

	// 편성창 열기 직전에 보고 있던 원래 카메라(로비) 기억용
	UPROPERTY()
	TObjectPtr<AActor> OriginalViewTarget = nullptr;
#pragma endregion 데이터 소스

#pragma region 내부 상태
private:
	/** @brief 현재 UI 상태 (일반/교체) */
	ESquadUIState CurrentState = ESquadUIState::Normal;

	/** @brief 현재 활성화된 탭 인덱스 */
	int32 CurrentTabIndex = SquadTabs::Character;

	/** @brief 현재 선택된 편성 슬롯 인덱스 (장비 교체 시 대상 식별용) */
	int32 SelectedFormationSlotIndex = -1;

	/** @brief 탭 전환 시 복구할 슬롯 메모리 (기본값: 첫 번째 슬롯) */
	int32 LastSelectedCharacterSlot = 0; // 캐릭터(0~2) 중 마지막 선택
	int32 LastSelectedUnitSlot = 3;      // 유닛(3~7) 중 마지막 선택

	/** @brief 교체를 위해 인벤토리에서 선택한 아이템 (확인 버튼 누르기 전 대기 상태) */
	FSquadItemUIData PendingSelection;

	/** @brief 현재 선택된 슬롯을 기준으로 디테일 창을 싹 새로고침해 주는 전용 헬퍼 함수 */
	void RefreshDetailPanelForCurrentSlot();

	/** @brief 현재 편성에 장착된 ID 목록 (인벤토리 테두리 표시용) */
	TArray<FName> CurrentEquippedIDs;

	UPROPERTY()
	bool bAutoSaveOnBack = true;
#pragma endregion 내부 상태

#pragma region 델리게이트
public:
	/** @brief 뒤로가기 요청 시 발생하는 이벤트 (LobbyHUD에서 구독) */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSquadBackRequested OnBackRequested;
#pragma endregion 델리게이트
};
