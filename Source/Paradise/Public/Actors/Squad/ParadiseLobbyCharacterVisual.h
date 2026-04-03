// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParadiseLobbyCharacterVisual.generated.h"

#pragma region 전방 선언
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequence;
#pragma endregion 전방 선언

/**
 * @class AParadiseLobbyCharacterVisual
 * @brief 로비 및 편성창에서 캐릭터의 외형만 보여주기 위한 초경량 마네킹 액터 (Dumb View)
 */
UCLASS()
class PARADISE_API AParadiseLobbyCharacterVisual : public AActor
{
	GENERATED_BODY()
	
public:
	AParadiseLobbyCharacterVisual();

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 마네킹에 스켈레탈 메시와 대기 애니메이션을 입힙니다.
	 * @param InMesh 덮어씌울 캐릭터 메시 (nullptr이면 마네킹을 숨김)
	 * @param InAnim 재생할 대기(Idle) 애니메이션
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Visual")
	void SetVisual(USkeletalMesh* InMesh, UAnimSequence* InAnim);
#pragma endregion 외부 인터페이스

#pragma region 컴포넌트
protected:
	/** @brief 최상위 루트 (트랜스폼용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DefaultRoot = nullptr;

	/** @brief 캐릭터 외형 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent = nullptr;
#pragma endregion 컴포넌트
};
