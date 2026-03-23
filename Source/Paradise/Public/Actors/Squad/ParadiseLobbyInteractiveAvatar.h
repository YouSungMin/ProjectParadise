// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParadiseLobbyInteractiveAvatar.generated.h"

#pragma region 전방 선언
class USkeletalMeshComponent;
class USplineComponent;
class UPrimitiveComponent;
class UAnimInstance;
#pragma endregion 전방 선언

/**
 * @class AParadiseLobbyInteractiveAvatar
 * @brief 로비 화면에서 메인 캐릭터를 표시하고, 상호작용(터치) 시 스플라인을 따라 이동하는 뷰/컨트롤러 클래스
 * @details 시각적 표현과 상호작용 로직만 처리하며, 데이터는 GameInstance의 Data Table을 참조합니다.
 */
UCLASS()
class PARADISE_API AParadiseLobbyInteractiveAvatar : public AActor
{
	GENERATED_BODY()
	
public:	
	AParadiseLobbyInteractiveAvatar();

#pragma region 생명주기
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	virtual void Tick(float DeltaTime) override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 현재 편성된 메인 캐릭터 데이터를 읽어와 메시와 애니메이션 시퀀스를 갱신합니다.
	 * @details SquadSubsystem에서 ID를 가져와 FGachaPoolRow 데이터를 기반으로 View를 업데이트합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Lobby")
	void RefreshMainCharacter();
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
protected:
	/**
	 * @brief 캐릭터 메시를 마우스로 클릭했을 때 호출되는 이벤트 함수
	 * @param TouchedComponent 터치된 컴포넌트
	 * @param ButtonPressed 눌린 마우스 버튼
	 */
	UFUNCTION()
	void OnCharacterClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

	/**
	 * @brief 캐릭터 메시를 모바일에서 터치했을 때 호출되는 이벤트 함수
	 * @param FingerIndex 터치한 손가락 인덱스
	 * @param TouchedComp 터치된 컴포넌트
	 */
	UFUNCTION()
	void HandleTouchBegin(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComp);

	/**
	 * @brief 스플라인 거리 기준으로 메시 위치와 회전을 업데이트하는 공통 함수
	 * @param Distance 현재 스플라인 이동 거리
	 * @param DeltaTime 프레임 델타 타임
	 */
	void UpdateMeshAlongSpline(float Distance, float DeltaTime);

private:
	/** @brief SquadSubsystem 플레이어 슬롯 변경 시 호출 */
	UFUNCTION()
	void OnPlayerSlotChanged(int32 SlotIndex, FName NewPlayerID);

	/**
	 * @brief 스플라인 이동을 종료하고 초기 대기(Idle) 상태로 복귀시키는 함수
	 */
	void ResetToIdleState();

	/**
	 * @brief 달리기(Run) 로직과 애니메이션을 시작하는 공통 내부 함수
	 */
	void StartRunning();
#pragma endregion 내부 로직

#pragma region 컴포넌트
protected:
	/** @brief 액터의 기본 루트 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DefaultRoot = nullptr;

	/** @brief 캐릭터 모델링을 렌더링하고, 상호작용(터치) 판정도 직접 담당하는 뷰 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> CharacterMesh = nullptr;

	/** @brief 터치 시 캐릭터가 이동할 궤도를 정의하는 스플라인 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> SplinePath = nullptr;
#pragma endregion 컴포넌트

#pragma region 상태 및 데이터
protected:
	/** @brief 스플라인을 따라 이동할 때의 목표 이동 속도 (Unreal Unit/s) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float RunSpeed = 600.0f;

	/**
	 * @brief 스플라인 끝에서 이 거리 안으로 들어오면 감속 + 회전 시작 (언리얼 유닛)
	 * @details 값이 클수록 더 일찍 감속합니다. (권장: 200 ~ 500)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (ClampMin = "50.0", DisplayName = "감속 시작 거리"))
	float DecelerationDistance = 200.0f;

	/**
	* @brief Idle 상태에서 캐릭터가 바라볼 월드 기준 Yaw 방향
	* @details 카메라를 정면으로 바라보게 하려면 이 값을 에디터에서 조정하세요.
	*          예: 카메라가 -Y 방향에 있으면 90.0f
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (DisplayName = "Idle 바라볼 Yaw 방향"))
	float IdleFacingYaw = 100.0f;

	/**
	 * @brief 스플라인 이동 중 메시 방향 보정 오프셋 Yaw 
	 * @details 캐릭터가 옆으로 달린다면 이 값을 90 또는 -90으로 조정하세요.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (DisplayName = "이동 방향 보정 오프셋 Yaw"))
	float MovementYawOffset = -90.0f;

	/**
	 * @brief Idle 복귀 후 카메라 정면을 바라볼 때 회전 보간 속도
	 * @details 낮을수록 천천히 돌아봅니다. (권장: 2.0 ~ 5.0)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (ClampMin = "0.1", DisplayName = "Idle 회전 보간 속도"))
	float IdleRotationInterpSpeed = 3.0f;

private:
	/** @brief 현재 캐릭터가 달리고 있는지 여부를 나타내는 상태 플래그 */
	UPROPERTY()
	bool bIsRunning = false;

	/**
	 * @brief Idle 도달 후 카메라 정면을 부드럽게 바라보는 중인지 여부
	 * @details 회전이 완료되면 false로 전환됩니다.
	 */
	bool bIsRotatingToIdle = false;

	/**
	 * @brief 스플라인 경로를 따라 누적된 현재 이동 거리
	 * @details 매 프레임(Tick)마다 RunSpeed * DeltaTime 만큼 증가하며, 위치를 계산하는 기준 좌표로 사용됩니다.
	 */
	UPROPERTY()
	float CurrentDistanceAlongSpline = 0.0f;

	/** @brief 데이터 테이블에서 비동기 로드하여 캐싱해둔 대기 애니메이션 (최적화용) */
	UPROPERTY()
	TObjectPtr<UAnimSequence> CachedIdleAnim = nullptr;

	/** @brief 데이터 테이블에서 비동기 로드하여 캐싱해둔 달리기 애니메이션 (최적화용) */
	UPROPERTY()
	TObjectPtr<UAnimSequence> CachedRunAnim = nullptr;
#pragma endregion 상태 및 데이터
};
