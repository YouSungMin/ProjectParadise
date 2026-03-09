// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Enums/GameEnums.h" 
#include "Data/Structs/GachaTypes.h"          
#include "ParadiseGachaBoxActor.generated.h"

#pragma region 전방 선언
class USkeletalMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInstance;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class UNiagaraSystem;
class AParadiseGachaItemActor;
class APlayerController;
class ACineCameraActor;
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

	/**
	* @brief 연출 종료 후 구슬을 제거하고 다음 뽑기를 위해 상태를 초기화합니다.
	* @details 박스 자신은 레벨에 유지됩니다. (레벨 시퀀스 바인딩 보존)
	*/
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void ResetState();

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

#pragma region 컴포넌트 및 에셋 설정
protected:
	/** @brief 상자 스켈레탈 메시 (애니메이션 재생 + 터치 감지) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Components")
	TObjectPtr<USkeletalMeshComponent> BoxMesh = nullptr;

	/** @brief 상자 주변 바닥을 은은하게 비출 라이트 (Idle 연출용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Components")
	TObjectPtr<UPointLightComponent> AuraLight = nullptr;

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

	/**
	 * @brief [1연차 전용] 구슬이 착지할 위치 오프셋 (박스 기준 상대 좌표)
	 * @details X = 앞뒤, Y = 좌우, Z = 높이. 기획자가 에디터에서 조절합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning")
	FVector SinglePullLandingOffset = FVector(0.0f, 0.0f, 0.0f);

	/**
	 * @brief [1연차 전용] 구슬이 하늘로 치솟는 포물선 최고 높이
	 * @details 멀티 뽑기보다 훨씬 높게 설정해 드라마틱한 연출을 만듭니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "100.0"))
	float SinglePullArcHeight = 1500.0f;

	/** @brief 구슬 비행 시간 최솟값 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "0.1"))
	float FlightTimeMin = 0.8f;

	/** @brief 구슬 비행 시간 최댓값 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "0.1"))
	float FlightTimeMax = 1.2f;

	/** @brief [10연차 전용] 구슬 순차 발사 간격 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "0.0"))
	float MultiSpawnInterval = 0.15f;

	// ── 결과창 딜레이 ────────────────────────────────────────

	/**
	 * @brief 마지막 구슬 리빌 후 결과창이 뜨기까지 대기 시간 (초)
	 * @details 더블 터치 스킵 시에는 이 딜레이 없이 즉시 표시됩니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Result", meta = (ClampMin = "0.0"))
	float ResultDelaySeconds = 2.0f;

	// ── 터치 입력 설정 ───────────────────────────────────────

	/** @brief 더블 터치 인식 허용 간격 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Input", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float DoubleTapThreshold = 0.3f;

	// ── 발광(Glow) 설정 ──────────────────────────────────────

	// ── 발광(Glow) 및 라이트 설정 (데이터 주도적) ──
	/** @brief Idle 상태일 때 켜지는 주변 빛의 세기 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow", meta = (ClampMin = "0.0"))
	float IdleLightIntensity = 5000.0f;

	/** @brief 터치 시 발광 증가 속도 (초당 강도) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow", meta = (ClampMin = "0.1"))
	float GlowIncreaseSpeed = 1.5f;

	/** @brief 터치 해제 시 발광 감소 속도 (초당 강도) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow", meta = (ClampMin = "0.1"))
	float GlowDecreaseSpeed = 0.5f;

	/** @brief 발광 최대 강도 (머티리얼 GlowIntensity 파라미터로 전달) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow", meta = (ClampMin = "0.0"))
	float MaxGlowIntensity = 3.0f;

	/** @brief 머티리얼 발광 강도 파라미터 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow")
	FName GlowIntensityParamName = FName(TEXT("GlowIntensity"));

	/** @brief 머티리얼 틈새 발광 색상 파라미터 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Glow")
	FName GlowColorParamName = FName(TEXT("GlowColor"));

	// ── 비주얼 에셋 맵 ──────────────────────────────────────

	/** @brief 등급별 틈새 발광 및 주변 라이트 색상 맵  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Visual")
	TMap<EItemRarity, FLinearColor> GlowColorsByRarity;

	/** @brief 등급별 뚜껑 오픈 폭발 이펙트 맵 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Visual")
	TMap<EItemRarity, TObjectPtr<UNiagaraSystem>> ClimaxEffectsByRarity;

	/** @brief 등급별 구슬 실루엣 머티리얼 맵 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Visual")
	TMap<EItemRarity, TObjectPtr<UMaterialInstance>> SilhouetteMaterialsByRarity;
#pragma endregion 컴포넌트 및 에셋 설정

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

#pragma region 내부 리빌 카운트 로직
private:
	/**
	 * @brief 구슬 하나가 리빌됐을 때 호출 (각 ItemActor 의 OnItemRevealed 에 바인딩)
	 * @param ItemData 리빌된 아이템 데이터
	 */
	UFUNCTION()
	void OnItemRevealedCallback(const FGachaResult& ItemData);

	/** @brief 모든 구슬 리빌 완료 → ResultDelaySeconds 후 결과창 */
	UFUNCTION()
	void ShowResultScreen();
#pragma endregion 내부 리빌 카운트 로직

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

	/** @brief 결과창 지연 타이머 핸들 */
	FTimerHandle ResultDelayTimerHandle;

	/** @brief 리빌 완료된 구슬 수 */
	int32 RevealedItemCount = 0;

	/** @brief 이번 뽑기 총 구슬 수 */
	int32 TotalItemCount = 0;

	/**
	 * @brief 현재 살아있는 구슬 액터 목록
	 * @details 꾹 누름 시 모든 구슬에 배속을 전파하기 위해 유지합니다.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AParadiseGachaItemActor>> SpawnedItems;
#pragma endregion 내부 상태

protected:
	/**
	 * @brief 구슬 착지 시 바닥에서 띄울 Z 오프셋 (구슬 절반 높이)
	 * @details 구슬이 바닥에 반쯤 잠기면 이 값을 높여주세요.
	 */
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Spawning", meta = (ClampMin = "0.0"))
		float OrbLandingZOffset = 50.0f;

};