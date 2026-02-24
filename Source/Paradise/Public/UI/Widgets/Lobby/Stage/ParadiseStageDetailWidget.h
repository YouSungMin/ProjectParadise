// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseStageDetailWidget.generated.h"

#pragma region 전방 선언
class UButton;
class UTextBlock;
class UWrapBox;
class UParadiseSquadFormationWidget;
class UParadiseGameInstance;
class UParadiseEnemyIconWidget;
class ALobbyPlayerController;
#pragma endregion 전방 선언

/** @brief 상세 팝업이 닫힐 때(Close 또는 Formation 이동 등) 부모에게 알림 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStageDetailClosed);

/**
 * @class UParadiseStageDetailWidget
 * @brief 스테이지 진입 전 상세 정보 팝업 (스쿼드 확인, 적 정보 표기, 진입 제어)
 */
UCLASS()
class PARADISE_API UParadiseStageDetailWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/** @brief 팝업을 열 때 StageSelectWidget에서 호출해 줄 초기화 함수 */
	void InitDetailPopup(FName InStageID);

	/** @brief 팝업 종료 소식을 외부에 알리는 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnStageDetailClosed OnDetailClosed;

#pragma region 내부 로직
private:
	UFUNCTION()
	void OnClickClose();

	UFUNCTION()
	void OnClickFormation();

	UFUNCTION()
	void OnClickEnterBattle();

	/** @brief 데이터 테이블에서 적 정보를 읽어와 WrapBox에 뿌려줌 */
	void SetupEnemyList(FName InStageID);
#pragma endregion 내부 로직

#pragma region UI 컴포넌트
protected:
	/** @brief 기존 편성창 위젯 재활용 (장비 탭 숨김 처리용 플래그 필요) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseSquadFormationWidget> UI_SquadPreview = nullptr;

	/** @brief 이 스테이지에 등장하는 적들의 슬롯이 추가될 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> WrapBox_EnemyList = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StageTitle = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Close = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Formation = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_EnterBattle = nullptr;

	/**
	 * @brief 적 아이콘을 생성하기 위한 위젯 클래스
	 * @note 에디터 디테일 패널에서 반드시 'WBP_EnemyIcon'을 할당해야 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI")
	TSubclassOf<UParadiseEnemyIconWidget> EnemyIconClass = nullptr;
#pragma endregion UI 컴포넌트

private:
	/** @brief 현재 선택된 스테이지 ID 캐싱 */
	FName CachedStageID = NAME_None;

	/** * @brief 잦은 호출 방지를 위한 게임 인스턴스 캐싱 (O(1) 속도 보장) */
	TWeakObjectPtr<UParadiseGameInstance> CachedGI = nullptr;

	/** @brief 메뉴 전환을 위한 로비 컨트롤러 캐싱 (O(1) 속도 보장) */
	TWeakObjectPtr<ALobbyPlayerController> CachedLobbyPC = nullptr;
};
