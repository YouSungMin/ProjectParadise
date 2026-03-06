// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LobbySetupActor.generated.h"

#pragma region 전방 선언
class ACameraActor;
#pragma endregion 전방 선언

/**
 * @class ALobbySetupActor
 * @brief 로비 레벨에 배치되어, 레벨 내 주요 오브젝트(카메라 등)의 참조를 들고 있는 액터.
 * @details
 * 1. 태그 검색(GetAllActors...) 대신 이 액터에 직접 카메라를 할당하여 최적화합니다.
 * 2. PlayerController가 시작될 때 이 액터를 참조하여 카메라 정보를 가져갑니다.
 */
UCLASS()
class PARADISE_API ALobbySetupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALobbySetupActor();

#pragma region 레벨 설정 데이터
public:
	/** @brief 메인 로비 카메라 (3D 캐릭터 뷰) */
	UPROPERTY(EditInstanceOnly, Category = "Paradise|Lobby Setup")
	TObjectPtr<ACameraActor> Camera_Main;

	/** @brief 전투(작전 지도) 카메라 */
	UPROPERTY(EditInstanceOnly, Category = "Paradise|Lobby Setup")
	TObjectPtr<ACameraActor> Camera_Battle;

	/** @brief 소환(가챠) 카메라 액터*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Lobby Setup")
	TObjectPtr<ACameraActor> Camera_Summon;

	/** @brief 가챠 연출(상자 낙하 및 오픈) 전용 시네 카메라 액터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Camera")
	TObjectPtr<ACameraActor> Camera_GachaAction = nullptr;

	// 추후 확장: 소환 연출용 카메라, 강화실 카메라 등등
	// UPROPERTY(EditInstanceOnly, Category = "Paradise|Lobby Setup")
	// TObjectPtr<ACameraActor> Camera_Summon;
#pragma endregion 레벨 설정 데이터

};
