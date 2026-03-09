// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParadiseMapEnvironmentActor.generated.h"

#pragma region 전방 선언
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;
#pragma endregion 전방 선언

/**
 * @class AParadiseMapEnvironmentActor
 * @brief 로비/스테이지 선택 화면의 3D 맵 배경(지도 널빤지)을 전담하는 액터입니다.
 * @details MVC 패턴 중 View/Model 에 해당하며, 컨트롤러의 지시에 따라 지도 텍스처를 동적으로 교체합니다 (SRP).
 */
UCLASS()
class PARADISE_API AParadiseMapEnvironmentActor : public AActor
{
	GENERATED_BODY()
	
#pragma region 초기화 및 생명주기
public:
	AParadiseMapEnvironmentActor();

protected:
	virtual void BeginPlay() override;
#pragma endregion 초기화 및 생명주기

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 지도 배경 텍스처를 교체합니다.
	 * @param NewMapTexture 새롭게 입힐 챕터 지도 텍스처 (데이터 테이블에서 전달됨)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Environment")
	void ChangeMapBackground(UTexture2D* NewMapTexture);
#pragma endregion 외부 인터페이스

#pragma region 컴포넌트
protected:
	/** @brief 지도를 그려낼 널빤지(Plane) 스태틱 메시 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Components")
	TObjectPtr<UStaticMeshComponent> MapMeshComponent = nullptr;
#pragma endregion 컴포넌트

#pragma region 내부 상태 및 설정
protected:
	/**
	 * @brief 머티리얼에서 텍스처를 교체할 파라미터의 이름 (기본값: "MapImage")
	 * @details 아트 팀이 머티리얼을 만들 때 지정한 Texture2D 파라미터 이름과 일치해야 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Config")
	FName TextureParameterName = FName(TEXT("MapImage"));

private:
	/**
	 * @brief 런타임에 텍스처를 교체하기 위해 캐싱해두는 다이내믹 머티리얼 인스턴스 (최적화)
	 * @details 매번 생성하지 않고 BeginPlay에서 한 번만 만들어 재사용합니다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMapMaterial = nullptr;
#pragma endregion 내부 상태 및 설정

};
