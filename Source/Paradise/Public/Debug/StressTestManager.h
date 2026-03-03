// StressTestManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StressTestManager.generated.h"

UCLASS()
class PARADISE_API AStressTestManager : public AActor
{
	GENERATED_BODY()

public:
	AStressTestManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// 테스트용 유닛 스폰 클래스
	UPROPERTY(EditAnywhere, Category = "Config")
	TSubclassOf<class APlayerBase> TestUnitClass;

	// 스폰할 유닛 수 (예: 100~500마리)
	UPROPERTY(EditAnywhere, Category = "Config")
	int32 SpawnCount = 100;

	// 매 프레임 무기를 교체할 것인가? (부하 테스트용)
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bTortureMode = false;

private:
	TArray<class APlayerBase*> SpawnedUnits;
};