// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FamiliarSpawner.generated.h"

UCLASS()
class PARADISE_API AFamiliarSpawner : public AActor
{
	GENERATED_BODY()

public:
	AFamiliarSpawner();

protected:
	virtual void BeginPlay() override;

	// 스폰할 유닛 클래스
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<class AUnitBase> UnitClass;

	// 랜덤 스폰 범위
	UPROPERTY(EditAnywhere, Category = "Spawning")
	FVector SpawnExtent = FVector(500.f, 500.f, 0.f);

	// 미리 풀에 생성해둘 개수
	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 PreSpawnCount = 5;

public:
	// 외부(UI/키보드)에서 호출할 스폰 함수
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnFamiliarByID(FName UnitID);

protected:
	// 위치 계산 유틸리티 함수
	FVector GetRandomSpawnLocation();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};