// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Enums/GameEnums.h" // EItemRarity가 있는 헤더
#include "Data/Structs/GachaTypes.h"          // FGachaResult가 있는 헤더
#include "ParadiseGachaBoxActor.generated.h"

#pragma region 전방 선언
class UStaticMeshComponent;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class UNiagaraSystem;
class UMaterialInstance;
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 가챠 연출을 시작합니다.
	 * @param InResults 서버/서브시스템에서 결정된 FGachaResult 결과 배열
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void PlayGachaSequence(const TArray<FGachaResult>& InResults);

	/** @brief 터치 유지 시 연출 배속 변경 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void SetGachaPlaySpeed(float SpeedMultiplier);

	/** @brief 연출 즉시 스킵 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void SkipGachaSequence();

	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnSingleCharacterRevealed OnSingleCharacterRevealed;

	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnGachaResultScreenRequested OnGachaResultScreenRequested;
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/** @brief 넘겨받은 결과 중 가장 높은 등급 판별 */
	EItemRarity GetHighestRarity(const TArray<FGachaResult>& Results) const;

	/** @brief 최고 등급에 따른 상자 발광 머티리얼 변경 */
	void UpdateBoxMaterialByRarity(EItemRarity HighestRarity);

	/** @brief 이벤트 트랙에서 호출될 등급별 폭발 이펙트 스폰 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Summon")
	void SpawnClimaxEffect();

	/** @brief 시퀀스 최종 종료 시 호출 */
	UFUNCTION()
	void HandleSequenceFinished();
#pragma endregion 내부 로직

#pragma region 컴포넌트 및 에셋
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Components")
	TObjectPtr<UStaticMeshComponent> BoxMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Sequence")
	TObjectPtr<ULevelSequence> SingleGachaSequence = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Sequence")
	TObjectPtr<ULevelSequence> MultiGachaSequence = nullptr;

	/** @brief 🚨 EItemRarity를 키 값으로 사용하는 머티리얼 맵 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Visual")
	TMap<EItemRarity, TObjectPtr<UMaterialInstance>> BoxMaterialsByRarity;

	/** @brief 🚨 EItemRarity를 키 값으로 사용하는 이펙트 맵 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Summon|Visual")
	TMap<EItemRarity, TObjectPtr<UNiagaraSystem>> ClimaxEffectsByRarity;
#pragma endregion 컴포넌트 및 에셋

#pragma region 내부 상태
private:
	/** @brief 현재 가챠의 최종 결과 (사용자의 구조체 타입 적용) */
	TArray<FGachaResult> CachedResults;

	/** @brief 이번 뽑기의 최고 등급 캐싱 */
	EItemRarity CachedHighestRarity = EItemRarity::Common;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> SequencePlayer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor = nullptr;
#pragma endregion 내부 상태
};