// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/StageStructs.h"
#include "UnitSpawner.generated.h"

USTRUCT(BlueprintType)
struct FWaveConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName UnitRowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 SpawnCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float SpawnInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float NextWaveDelay;

	FWaveConfig()
		: UnitRowName(NAME_None), SpawnCount(10), SpawnInterval(1.0f), NextWaveDelay(5.0f)
	{
	}
};

UCLASS()
class PARADISE_API AUnitSpawner : public AActor
{
	GENERATED_BODY()

public:
	AUnitSpawner();

protected:
	virtual void BeginPlay() override;

	/** @brief 스폰할 유닛의 베이스 클래스 */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<class AUnitBase> UnitClass;

	/** @brief 스폰 범위 */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	FVector SpawnExtent = FVector(500.f, 500.f, 0.f);

	/** @brief 이 스포너가 담당할 스테이지 ID (예: Stage1_1) */
	UPROPERTY(EditAnywhere, Category = "Spawning|Data")
	FName TargetStageID;

	/** @brief 테이블에서 로드된 웨이브 정보 (에디터 확인용) */
	UPROPERTY(VisibleAnywhere, Category = "Spawning")
	TArray<FWaveConfig> WaveConfigs;

	/** @brief 초기 풀링 생성 개수 */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 PreSpawnCount = 5;

	FTimerHandle SpawnTimerHandle;
	int32 CurrentWaveIndex = 0;
	int32 CurrentSpawnCountInWave = 0;
	FName EnemyRowName;

	/** @brief 실제 스폰 처리 함수 */
	void SpawnUnit();

	/** @brief 네비메시 기반 랜덤 위치 계산 */
	FVector GetRandomSpawnLocation();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};