// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/ObjectPoolSubsystem.h"
#include "Interfaces/ObjectPoolInterface.h"
#include "Paradise/Paradise.h"

AActor* UObjectPoolSubsystem::SpawnPooledActor(UClass* Class, FVector location,
	FRotator rotation, AActor* Owner, APawn* Instigator)
{
	if (!Class) return nullptr;

	FObjectPoolQueue& PoolQueue = PoolMap.FindOrAdd(Class);

	AActor* PooledActor = nullptr;

	//풀에 남는 게 있는지 확인 (유효하지 않은 건 버림)
	while (PoolQueue.Pool.Num() > 0)
	{
		AActor* Candidate = PoolQueue.Pool.Pop();
		if (IsValid(Candidate))
		{
			PooledActor = Candidate;
			UE_LOG(LogParadiseObjectPool, Warning, TEXT("♻️ [ObjectPool] 재사용 성공 (Reuse): %s (남은 개수: %d)"),
				*PooledActor->GetName(), PoolQueue.Pool.Num());
			break;
		}
	}

	//풀에 없으면 새로 생성
	if (!PooledActor)
	{
		FActorSpawnParameters Params;
		Params.Owner = Owner;
		Params.Instigator = Instigator;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		PooledActor = GetWorld()->SpawnActor<AActor>(Class, location, rotation, Params);

		if (PooledActor)
		{
			UE_LOG(LogParadiseObjectPool, Warning, TEXT("✨ [ObjectPool] 신규 생성 (New Spawn): %s"),
				*PooledActor->GetName());
		}
	}
	else
	{
		// 풀에서 꺼냈으면 위치/회전 강제 지정
		PooledActor->SetActorLocationAndRotation(location, rotation);
	}

	//활성화 처리 (인터페이스 호출)
	if (PooledActor && PooledActor->Implements<UObjectPoolInterface>())
	{
		//(초기화)
		IObjectPoolInterface::Execute_OnPoolActivate(PooledActor);
	}
	return PooledActor;
}

#pragma region 풀 초기화 구현
void UObjectPoolSubsystem::PreSpawnPool(UClass* Class, UWorld* World, int32 Count)
{
	// 방어 코드: 클래스 정보가 없거나, 월드가 없거나, 생성 개수가 0 이하면 무시
	if (!Class || !World || Count <= 0) return;

	// 1. 해당 클래스를 키(Key)로 하는 큐(Queue)를 맵에서 찾거나 새로 만듭니다.
	FObjectPoolQueue& Queue = PoolMap.FindOrAdd(Class);

	// 2. 요청한 개수만큼 반복해서 액터를 스폰합니다.
	for (int32 i = 0; i < Count; ++i)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // 충돌 무시하고 무조건 스폰

		// 보이지 않는 원점(0,0,0)에 액터를 생성합니다.
		AActor* SpawnedActor = World->SpawnActor<AActor>(Class, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (SpawnedActor)
		{
			// 3. 스폰되자마자 화면에 보이면 안 되므로, 인터페이스를 통해 즉시 비활성화(Deactivate) 시킵니다.
			if (SpawnedActor->Implements<UObjectPoolInterface>())
			{
				IObjectPoolInterface::Execute_OnPoolDeactivate(SpawnedActor);
			}
			else
			{
				// 인터페이스가 없는 일반 액터일 경우를 대비한 안전장치
				SpawnedActor->SetActorHiddenInGame(true);
				SpawnedActor->SetActorTickEnabled(false);
			}

			// 4. 비활성화된 액터를 큐(풀)에 집어넣어 대기시킵니다.
			Queue.Pool.Add(SpawnedActor);
		}
	}

	UE_LOG(LogParadiseObjectPool, Log, TEXT("[ObjectPool] %s 클래스 %d개 사전 스폰(Pre-spawn) 완료!"), *Class->GetName(), Count);
}
#pragma endregion 풀 초기화 구현

void UObjectPoolSubsystem::ReturnToPool(AActor* InActor)
{
	if (IsValid(InActor)) {
		if (InActor->Implements<UObjectPoolInterface>()) {
			IObjectPoolInterface::Execute_OnPoolDeactivate(InActor);
		}
		else {
			InActor->Destroy();
			UE_LOG(LogParadiseObjectPool, Error, TEXT("IObjectPoolInterface : 인터페이스 구현안되있음"));
			return;
		}

		FObjectPoolQueue& poolQueue = PoolMap.FindOrAdd(InActor->GetClass());
		poolQueue.Pool.Push(InActor);

		UE_LOG(LogParadiseObjectPool, Warning, TEXT("📥 [ObjectPool] 반납 완료 (Return): %s (현재 보유량: %d)"),
			*InActor->GetName(), poolQueue.Pool.Num());
	}

}