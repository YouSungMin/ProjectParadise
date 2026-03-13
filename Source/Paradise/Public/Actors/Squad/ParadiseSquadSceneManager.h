// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParadiseSquadSceneManager.generated.h"

#pragma region 전방 선언
class UCameraComponent;
class UArrowComponent;
class AParadiseLobbyCharacterVisual;
class USquadSubsystem;
class UParadiseGameInstance;
#pragma endregion 전방 선언

/**
 * @class AParadiseSquadSceneManager
 * @brief 편성창 3D 디오라마를 관리하는 스튜디오 매니저 (Controller)
 * @details 카메라 뷰와 3개의 스폰 포인트를 관리하며, 데이터를 읽어와 모델링을 갱신합니다.
 */
UCLASS()
class PARADISE_API AParadiseSquadSceneManager : public AActor
{
	GENERATED_BODY()
	
public:
	AParadiseSquadSceneManager();

protected:
	virtual void BeginPlay() override;

#pragma region 외부 인터페이스
public:
	/**
	 * @brief Subsystem의 현재 편성 데이터를 읽어와 3D 모델링을 전면 갱신합니다.
	 * @details 편성 UI가 열리거나, 교체가 일어났을 때 MainWidget에서 호출합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|SquadScene")
	void RefreshSquadScene();
#pragma endregion 외부 인터페이스

#pragma region 스튜디오 셋업 컴포넌트
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DefaultRoot = nullptr;

	/** @brief 편성 화면용 고정 카메라 (엔드필드 뷰) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> SceneCamera = nullptr;

	/** @brief 중앙(메인) 캐릭터 서 있을 위치와 방향 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> Point_Main = nullptr;

	/** @brief 좌측(서브1) 캐릭터 서 있을 위치와 방향 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> Point_Sub1 = nullptr;

	/** @brief 우측(서브2) 캐릭터 서 있을 위치와 방향 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> Point_Sub2 = nullptr;
#pragma endregion 스튜디오 셋업 컴포넌트

#pragma region 설정 및 상태
protected:
	/** @brief 생성할 마네킹 액터 클래스 (BP_LobbyCharacterVisual 할당) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<AParadiseLobbyCharacterVisual> VisualActorClass = nullptr;

private:
	/** @brief 인덱스별 마네킹 액터 캐싱 풀 (0: Main, 1: Sub1, 2: Sub2) */
	UPROPERTY()
	TMap<int32, AParadiseLobbyCharacterVisual*> SpawnedVisuals;

	/** @brief 특정 슬롯의 데이터를 읽어 마네킹 옷을 갈아입히는 내부 로직 */
	void UpdateSlotVisual(int32 SlotIndex, FName CharacterID, UArrowComponent* TargetPoint);
#pragma endregion 설정 및 상태

};
