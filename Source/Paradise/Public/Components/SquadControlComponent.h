// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SquadControlComponent.generated.h"

class APlayerBase;
class AInGameController;
class AAIController;

/** @brief 캐릭터 교체 완료 시 방송 (교체된 캐릭터 인덱스 전달) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerSwitched, int32, NewCharacterIndex);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARADISE_API USquadControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USquadControlComponent();

		
public:

	/** @brief 플레이어 컨트롤러 Getter */
	AInGameController* GetOwnerPC() const;

	/** @brief Squad액터 배열 Getter */
	const TArray<TObjectPtr<APlayerBase>>& GetActiveSquadPawns() const;

	/** @brief 현재 컨트롤 중인 캐릭터 인덱스 Getter */
	int32 GetCurrentControlledIndex() const;

	/** @brief 카메라 매니저 Getter 함수 */
	class AParadiseCameraManager* GetParadiseCameraManager();

	/*
	* @brief 게임 시작 시 스쿼드 3명을 월드에 스폰하고 초기화하는 함수
	* @details PlayerState의 데이터를 기반으로 실제 육체(Pawn)를 생성합니다.
	*/
	void InitializeSquadPawns();

	/*
	 * @brief 현재 조종하지 않는 캐릭터에게 AI 컨트롤러를 빙의시키는 함수
	 */
	void PossessAI(APlayerBase* TargetCharacter);

	/** @brief 사망 시 다음 생존 캐릭터를 찾아 전환을 시도합니다. 생존 캐릭터가 없다면 false 반환. */
	bool SwitchToNextSurvivor();

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

public:

	/** @brief 모든 영웅이 사망했는지 여부 */
	bool bIsSquadWipedOut = false;

	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnPlayerSwitched OnPlayerSwitched;

protected:

	//  데이터 및 설정 (Data & Config)

	/** @brief 카메라 매니저 찾아둘 약참조 포인터 */
	TWeakObjectPtr<class AParadiseCameraManager> CachedCameraManager = nullptr;

	/*
	 * @brief 동료(AI) 영웅에게 할당할 AI 컨트롤러 클래스
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	TSubclassOf<AAIController> SquadAIControllerClass = nullptr;

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

	/* * @brief 스폰할 캐릭터베이스 클래스 (BP_PlayerBase 할당용)
	* @details 여기에 에디터에서 만든 캐릭터 블루프린트를 할당
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Test")
	TSubclassOf<APlayerBase> PlayerBaseClass = nullptr;
};
