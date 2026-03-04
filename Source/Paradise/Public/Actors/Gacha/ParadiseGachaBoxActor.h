// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Enums/GameEnums.h" // EItemRarity가 있는 헤더
#include "Data/Structs/GachaTypes.h"          // FGachaResult가 있는 헤더
#include "ParadiseGachaBoxActor.generated.h"

#pragma region 전방 선언
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInstance;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class UNiagaraSystem;
class AParadiseGachaItemActor;
class APlayerController;
#pragma endregion 전방 선언

/** @brief 1회 소환용: 한 명의 캐릭터 리빌(실루엣 해제) 시점 알림 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSingleCharacterRevealed, const FGachaResult&, CharacterData);

/** @brief 결과창 호출 델리게이트 (1회/10회 모두 최종에 호출) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGachaResultScreenRequested, const TArray<FGachaResult>&, GachaResults);

/**
 * @class AParadiseGachaBoxActor
 * @brief 가챠(소환) 연출을 전담하는 박스 액터
 */
UCLASS()
class PARADISE_API AParadiseGachaBoxActor : public AActor
{
	GENERATED_BODY()

public:
	AParadiseGachaBoxActor();

#pragma region 생명주기
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#pragma endregion 생명주기

	// ─────────────────────────────────────────────────────────────
#pragma region 외부 인터페이스
public:
	/**
	 * @brief 가챠 연출을 시작합니다. (GachaSubsystem → 이 함수 호출)
	 * @param InResults 서버/서브시스템에서 결정된 뽑기 결과 배열
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void PlayGachaSequence(const TArray<FGachaResult>& InResults);

	/** @brief 연출 즉시 스킵 → 결과창 바로 표시 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void SkipGachaSequence();

	/** @brief 현재 재생 속도 배율 설정 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void SetGachaPlaySpeed(float SpeedMultiplier);

	/** @brief 단일 캐릭터 리빌 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnSingleCharacterRevealed OnSingleCharacterRevealed;

	/** @brief 최종 결과창 요청 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnGachaResultScreenRequested OnGachaResultScreenRequested;
#pragma endregion 외부 인터페이스

	// ─────────────────────────────────────────────────────────────
#pragma region 컴포넌트 및 에셋 설정 (기획자 노출)
protected:
	/** @brief 상자 스켈레탈 메시 (애니메이션 재생 + 터치 감지) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Components")
	TObjectPtr<USkeletalMeshComponent> BoxMesh = nullptr;

	// ── 시퀀스 에셋 ─────────────────────────────────────────

	/** @brief [공용] 낙하 + 착지 시퀀스 → 1회 재생 후 자동으로 Idle 전환 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Sequence")
	TObjectPtr<ULevelSequence> IntroSequence = nullptr;

	/** @brief [공용] 착지 후 은은한 흔들흔들 루프 시퀀스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Sequence")
	TObjectPtr<ULevelSequence> IdleShakeSequence = nullptr;

	/** @brief [1연차] 격렬한 흔들 + 뚜껑 열림 시퀀스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Sequence")
	TObjectPtr<ULevelSequence> OpenSingleSequence = nullptr;

	/** @brief [10연차] 격렬한 흔들 + 뚜껑 열림 시퀀스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Sequence")
	TObjectPtr<ULevelSequence> OpenMultiSequence = nullptr;

	// ── 구슬 스폰 설정 ───────────────────────────────────────

	/** @brief 스폰할 구슬 블루프린트 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning")
	TSubclassOf<AParadiseGachaItemActor> ItemActorClass;

	/** @brief 10연차 구슬이 퍼지는 원형 반경 (언리얼 유닛) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "100.0"))
	float EruptRadius = 350.0f;

	/** @brief 구슬 포물선 최고 높이 (언리얼 유닛) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "50.0"))
	float EruptArcHeight = 600.0f;

	/** @brief 구슬 비행 시간 최솟값 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "0.1"))
	float FlightTimeMin = 0.8f;

	/** @brief 구슬 비행 시간 최댓값 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "0.1"))
	float FlightTimeMax = 1.2f;

	/** @brief [10연차 전용] 구슬 순차 발사 간격 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "0.0"))
	float MultiSpawnInterval = 0.15f;

	// ── 터치 입력 설정 ───────────────────────────────────────

	/** @brief 꾹 누름 시 재생 배속 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Input", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float PressPlayRate = 2.0f;

	/** @brief 더블 터치 인식 허용 간격 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Input", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float DoubleTapThreshold = 0.3f;

	// ── 발광(Glow) 설정 ──────────────────────────────────────

	/** @brief 터치 시 발광 증가 속도 (초당 강도) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow", meta = (ClampMin = "0.1"))
	float GlowIncreaseSpeed = 1.5f;

	/** @brief 터치 해제 시 발광 감소 속도 (초당 강도) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow", meta = (ClampMin = "0.1"))
	float GlowDecreaseSpeed = 0.5f;

	/** @brief 발광 최대 강도 (머티리얼 GlowIntensity 파라미터로 전달) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow", meta = (ClampMin = "0.0"))
	float MaxGlowIntensity = 3.0f;

	/** @brief 머티리얼 발광 파라미터 이름 (아티스트가 변경 가능) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow")
	FName GlowIntensityParamName = FName(TEXT("GlowIntensity"));

	// ── 비주얼 에셋 맵 ──────────────────────────────────────

	/** @brief 등급별 상자 머티리얼 맵 (상자 열릴 때 교체) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Visual")
	TMap<EItemRarity, TObjectPtr<UMaterialInstance>> BoxMaterialsByRarity;

	/** @brief 등급별 뚜껑 오픈 폭발 이펙트 맵 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Visual")
	TMap<EItemRarity, TObjectPtr<UNiagaraSystem>> ClimaxEffectsByRarity;

	/** @brief 등급별 구슬 실루엣 머티리얼 맵 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Visual")
	TMap<EItemRarity, TObjectPtr<UMaterialInstance>> SilhouetteMaterialsByRarity;
#pragma endregion 컴포넌트 및 에셋 설정 (기획자 노출)

	// ─────────────────────────────────────────────────────────────
#pragma region 내부 시퀀스 로직
private:
	/**
	 * @brief 시퀀스를 재생하는 내부 공통 함수 (람다 없음 — 상태머신으로 분기)
	 * @param Sequence  재생할 레벨 시퀀스 에셋
	 * @param Step      현재 단계 → CurrentStep에 저장되어 OnSequenceFinished에서 분기
	 * @param bLoop     루프 여부 (true = Idle 시퀀스)
	 */
	void PlaySequenceInternal(ULevelSequence* Sequence, EGachaSequenceStep Step, bool bLoop);

	/** @brief 현재 SequencePlayer를 안전하게 정지 및 해제 */
	void StopCurrentSequence();

	/**
	 * @brief 시퀀스 재생 완료 시 호출되는 단일 콜백
	 * @details CurrentStep 값을 보고 다음 단계로 전환합니다.
	 */
	UFUNCTION()
	void OnSequenceFinished();

	/** @brief 결과 배열에서 최고 등급 반환 */
	EItemRarity GetHighestRarity(const TArray<FGachaResult>& Results) const;

	/** @brief 상자 열리는 순간 등급 머티리얼 + 폭발 이펙트 적용 */
	void ApplyClimaxVisual();

	/**
	 * @brief 시퀀서 이벤트 트랙에서 호출 → 구슬 순차 스폰 시작
	 * @details 1연차: 즉시 1개 / 10연차: MultiSpawnInterval 간격으로 슝슝슝
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void EruptGachaItems();

	/**
	 * @brief 타이머 딜레이 후 호출되어 Index번째 구슬을 스폰
	 * @param Index CachedResults 배열 인덱스
	 */
	UFUNCTION()
	void SpawnSingleItem(int32 Index);
#pragma endregion 내부 시퀀스 로직

	// ─────────────────────────────────────────────────────────────
#pragma region 내부 터치 로직
private:
	/** @brief BoxMesh 마우스 클릭 이벤트 래퍼 */
	UFUNCTION()
	void HandleBoxClicked(UPrimitiveComponent* TouchedComp, FKey ButtonPressed);

	/** @brief BoxMesh 모바일 터치 시작 래퍼 → (꾹 누름 배속 시작) */
	UFUNCTION()
	void HandleBoxTouchBegin(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComp);

	/** @brief BoxMesh 모바일 터치 종료 래퍼 (탭 감지 + 배속 복귀) */
	UFUNCTION()
	void HandleBoxTouchEnd(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComp);

	/** @brief 터치/클릭 공통 진입점 (싱글/더블 분기) */
	void ProcessTouchInput();

	/** @brief 싱글 탭 → Idle 중단 + Open 시퀀스 재생 */
	void ProcessSingleTap();

	/** @brief 더블 탭 → 연출 즉시 스킵 */
	void ProcessDoubleTap();

	/**
	 * @brief Tick 에서 꾹 누름 상태 감지
	 * @details 모바일: bIsMobilePressing 플래그 사용
	 *          에디터(PC): PlayerController::IsInputKeyDown(LeftMouseButton) 폴링
	 */
	void TickPressUpdate();

	/** @brief 매 Tick 발광 강도 부드럽게 보간 */
	void TickGlowUpdate(float DeltaTime);
#pragma endregion 내부 터치 로직

	// ─────────────────────────────────────────────────────────────
#pragma region 내부 상태
private:
	/** @brief 현재 가챠 결과 캐시 */
	TArray<FGachaResult> CachedResults;

	/** @brief 이번 뽑기 최고 등급 캐시 */
	EItemRarity CachedHighestRarity = EItemRarity::Common;

	/** @brief 현재 재생 중인 시퀀스 단계 (OnSequenceFinished 분기용) */
	EGachaSequenceStep CurrentStep = EGachaSequenceStep::None;

	/** @brief 런타임 발광 제어용 다이나믹 머티리얼 인스턴스 */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BoxDynamicMat = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> SequencePlayer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor = nullptr;

	/** @brief 현재 발광 강도 (0.0 ~ 1.0, Tick 보간) */
	float ShakeIntensity = 0.0f;

	/**
	 * @brief 현재 누름 여부 (발광/배속 제어용)
	 * @details 모바일: TouchBegin/End 로 설정
	 *          PC/에디터: TickPressUpdate 에서 IsInputKeyDown 으로 설정
	 */
	bool bIsPressing = false;

	/** @brief 모바일 터치 누름 중 여부 (TouchBegin → true, TouchEnd → false) */
	bool bIsMobilePressing = false;

	/** @brief 이전 터치 시각 (더블 터치 감지용) */
	float LastTouchTime = -1.0f;

	/** @brief Open 시퀀스 재생 중 여부 (중복 입력 방지) */
	bool bIsOpening = false;

	/** @brief 10연차 구슬 순차 스폰 타이머 핸들 배열 */
	TArray<FTimerHandle> SpawnTimerHandles;
#pragma endregion 내부 상태

};