// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectPoolSubsystem.generated.h"

struct FActorSpawnParameters;

USTRUCT()
struct FObjectPoolQueue
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Pool;
};
/**
 * @brief 월드 기반 오브젝트 풀링 시스템을 관리하는 서브시스템입니다.
 * @note Actor의 재사용 및 관리를 담당합니다.
 * @see IObjectPoolInterface
 */
UCLASS()
class PARADISE_API UObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/**
	 * @brief 풀에서 액터를 가져올 때 특정 타입(T)으로 캐스팅하여 반환하는 템플릿 함수
	 * @tparam T      반환받을 액터의 구체적인 클래스 타입 (AActor 상속 필수)
	 * @param Class   스폰할 액터의 UClass 정보
	 * @param location 스폰할 월드 위치
	 * @param rotation 스폰할 월드 회전값
	 * @param Owner    액터의 소유자(Owner) 설정
	 * @param Instigator 액터의 가해자(Instigator) 설정
	 * @return T* 풀에서 꺼내지거나 새로 생성된, T 타입으로 캐스팅된 액터 포인터
	 */
	template<typename T>
	T* SpawnPoolActor(UClass* Class, FVector location, FRotator rotation, AActor* Owner, APawn* Instigator)
	{
		return Cast<T>(SpawnPooledActor(Class, location, rotation, Owner, Instigator));
	}

	/**
	 * @brief 오브젝트 풀을 조회하여 액터를 가져오거나, 없으면 새로 생성하는 핵심 함수
	 * @details 풀(Queue)에 유효한 액터가 있다면 재사용(Reuse)하고, 없다면 SpawnActor를 통해 새로 생성합니다.
	 * 가져온 액터에 대해 IObjectPoolInterface::OnPoolActivate를 호출합니다.
	 * @param Class    스폰할 대상 UClass
	 * @param location 초기 위치
	 * @param rotation 초기 회전
	 * @param Owner    소유자 액터
	 * @param Instigator 가해자 폰
	 * @return AActor* 활성화된 액터의 포인터 (실패 시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	AActor* SpawnPooledActor(UClass* Class, FVector location, FRotator rotation, AActor* Owner, APawn* Instigator);

	/**
	 * @brief 전투 전 렉(GC) 방지를 위해 지정된 개수만큼 액터를 미리 생성하여 풀에 보관합니다.
	 * @param Class 생성할 대상 액터의 클래스 (TSubclassOf)
	 * @param World 스폰을 수행할 월드 컨텍스트
	 * @param Count 미리 만들어둘 액터의 개수
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void PreSpawnPool(UClass* Class, UWorld* World, int32 Count);

	/**
	 * @brief 사용이 끝난 액터를 풀로 반납(비활성화)하는 함수
	 * @details 액터를 즉시 Destroy하지 않고 숨긴 뒤, 풀 큐에 넣습니다.
	 * 반납 전 IObjectPoolInterface::OnPoolDeactivate를 호출합니다.
	 * @param InActor 풀로 되돌릴 대상 액터
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void ReturnToPool(AActor* InActor);

private:

	/** * @brief 클래스 타입(UClass*)을 키(Key)로 하여 관리되는 오브젝트 풀 맵
	 */
	UPROPERTY()
	TMap<UClass*, FObjectPoolQueue> PoolMap;
};