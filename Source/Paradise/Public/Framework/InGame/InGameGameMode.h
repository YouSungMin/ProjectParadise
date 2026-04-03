// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/DataTable.h"
#include "Data/Enums/GameEnums.h"
#include "Data/Structs/StageStructs.h"
#include "Data/Structs/ResultUITypes.h"
#include "InGameGameMode.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageVictorySignature, const FStageClearRewardData&, RewardData);

/**
 * @class AInGameGameMode
 * @brief 게임의 규칙, 스테이지 진행, 페이즈 전환 로직을 총괄하는 클래스입니다.
 * @details 데이터 테이블에서 스테이지 정보를 로드하고, GameState와 협력하여 게임 흐름을 제어합니다.
 */
UCLASS()
class PARADISE_API AInGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/**
	* @brief 디버그 테스트 함수 콘솔창에서 호출시 강제 승리
	*/
	UFUNCTION(Exec)
	void ForceVictory();

	AInGameGameMode();

	virtual void BeginPlay() override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	/**
	 * @brief 게임모드 액터가 파괴되거나 레벨이 전환될 때 호출됩니다.
	 * @details 설정창을 통한 로비 강제 이탈 시 BGM을 끄기 위한 안전장치입니다.
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** 
	* @brief 스테이지 타이머가 1초 경과할 때마다 호출되는 함수
	*/
	void OnStageTimerElapsed();
	
public:

#pragma region PlayerSquad 셋업

	//0220 김성현 - 스쿼드 시스템 셋업 함수 추가
private:
	/** * @brief 접속한 플레이어의 편성(Squad) 데이터를 읽어와 인게임 육체와 영혼을 스폰합니다.
	 * @param NewPlayer 방금 접속을 완료한 플레이어 컨트롤러
	 */
	void SetupPlayerSquad(APlayerController* NewPlayer);

#pragma endregion PlayerSquad 셋업

#pragma region 스테이지 클리어 보상

	/** 스테이지 클리어 보상 지급 함수 */
	void DistributeStageRewards();

#pragma endregion 스테이지 클리어 보상

	/**
	 * @brief 게임 페이즈를 변경하고 관련 이벤트를 트리거합니다.
	 * @param NewPhase 변경할 새로운 게임 단계
	 */
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetGamePhase(EGamePhase NewPhase);

public:
	/**
	 * @brief 게임 승패가 결정되었을 때 호출하는 함수
	 * @details 타임오버, 기지 파괴 등 모든 게임 종료 상황에서 이 함수를 호출합니다.
	 * @param bIsVictory true: 플레이어 승리 / false: 플레이어 패배
	 */
	UFUNCTION(BlueprintCallable, Category = "GameRules")
	void EndStage(bool bIsVictory);

protected:
	/**
	 * @brief 데이터 테이블로부터 스테이지 정보를 읽어와 초기화합니다.
	 * @param StageID 로드할 스테이지의 고유 ID
	 */
	void InitializeStageData(FName StageID);

	/** @name Phase Transition Handlers
	 * 각 페이즈 진입 시 내부 로직을 처리하는 함수군입니다.
	 * @{ */
	void OnPhaseReady();	///< [준비] 카운트다운 처리
	void OnPhaseCombat();	///< [전투] 몬스터 스폰 및 전투 진행
	void OnPhaseVictory();	///< [승리] 승리 처리 및 결과창 준비
	void OnPhaseDefeat();	///< [패배] 패배 처리 및 결과창 준비
	void OnPhaseResult();	///< [결과] 결과창 표시 및 레벨 이동 준비
	/** @} */

public:

	/** @brief 스테이지 승리 보상 데이터를 UI로 전달하는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnStageVictorySignature OnStageVictory;

protected:
	/** @brief [캐싱] 전역 상태 관리를 위한 GameState 포인터 */
	UPROPERTY()
	class AInGameGameState* CachedGameState;

	/** @brief 현재 진행중인 스테이지 ID */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FName CurrentStageID;

	/** @brief [데이터] 현재 진행 중인 스테이지의 상세 스탯(시간, 보상 등) */
	FStageStats CurrentStageData;

	/** @brief [상태] 현재 게임 페이즈 단계 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	EGamePhase CurrentPhase;
	
	/** @brief [타이머] 스테이지 진행을 위한 타이머 핸들 */
	UPROPERTY()
	FTimerHandle StageTimerHandle;

	/** @brief [타이머] 결과 창 뜨기 전에 연출을 위한 타이머 핸들*/
	UPROPERTY()
	FTimerHandle ResultTimerHandle;

protected:
	/** @brief 전투 전 미리 생성해둘 데미지 텍스트 블루프린트 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Pool")
	TSubclassOf<class ADamageTextActor> DamageTextClass;

	/** @brief 사전 생성 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Pool", meta = (ClampMin = "10"))
	int32 PreSpawnDamageTextCount = 30;

};
