// Copyright (C) Project Paradise. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "LevelLoadingSubsystem.generated.h"

#pragma region 전방 선언
class ULoadingWidget;
class UUserWidget;
class UTexture2D;
#pragma endregion 전방 선언

/**
 * @class ULevelLoadingSubsystem
 * @brief 레벨 이동 간의 비동기 로딩 및 전이 맵(Transition Map) 흐름을 관리하는 서브시스템.
 * @details
 * 1. 요청 시 'LoadingMap'으로 즉시 이동하여 메모리를 확보합니다.
 * 2. 로딩 맵 진입 후 비동기 에셋 로딩을 시작하며 로딩 위젯을 표시합니다.
 * 3. 최소 로딩 시간(2초)과 실제 로딩 완료를 체크하여 최종 레벨로 이동합니다.
 */
UCLASS()
class PARADISE_API ULevelLoadingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	ULevelLoadingSubsystem();

	// 서브시스템 수명주기
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 레벨 이동 함수. (C++ 기본값 할당 제거 -> 문법 오류 해결)
	 * @param InTargetLevelName 목표 레벨
	 * @param InLoadingMapName 로딩 맵 이름 (None을 넣으면 내부에서 L_Loading으로 처리)
	 * @param InAssetsToPreload 미리 로딩할 에셋 목록
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|System|Loading", meta = (AutoCreateRefTerm = "InAssetsToPreload, InLoadingMapName"))
	void StartLevelTransition(
		FName InTargetLevelName, 
		FName InLoadingMapName, 
		const TArray<TSoftObjectPtr<UObject>>& InAssetsToPreload,
		TSoftObjectPtr<UTexture2D> InLoadingImage = nullptr);

	/**
	 * @brief 로딩 위젯 클래스를 설정합니다 (GameInstance 초기화 시 호출 권장).
	 * @param NewLoadingWidgetClass 사용할 위젯 클래스 (BP_LoadingWidget)
	 */
	void SetLoadingWidgetClass(TSubclassOf<UUserWidget> NewLoadingWidgetClass);
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/**
	 * @brief 맵이 로드된 직후 호출되는 델리게이트 콜백.
	 * @details 현재 맵이 로딩 맵(L_Loading)인지 확인하고, 맞다면 실제 로딩 프로세스를 시작합니다.
	 */
	void OnMapLoadComplete(UWorld* World);

	/** @brief 실제 비동기 로딩 및 타이머 시작. */
	void BeginAsyncLoading();

	/** @brief 0.05초마다 호출되어 로딩 진행률을 갱신하는 함수. */
	void UpdateLoadingProgress();

	/** @brief 로딩 완료 후 최종 레벨로 이동 및 정리. */
	void FinishLoading();
#pragma endregion 내부 로직

#pragma region 데이터 및 상태
private:
	/** @brief 비동기 로딩을 수행하는 엔진 매니저. */
	FStreamableManager StreamableManager;

	/** @brief 현재 진행 중인 로딩 핸들 (Smart Pointer). */
	TSharedPtr<FStreamableHandle> CurrentLoadHandle = nullptr;

	/** @brief 현재 띄워진 로딩 위젯 (Smart Pointer). */
	UPROPERTY()
	TObjectPtr<ULoadingWidget> CurrentLoadingWidget = nullptr;

	/** @brief 로딩 위젯 생성을 위한 클래스 정보. */
	UPROPERTY()
	TSubclassOf<UUserWidget> LoadingWidgetClass = nullptr;

	/** @brief 타이머 핸들. */
	FTimerHandle ProgressTimerHandle;

	/** @brief 이동해야 할 최종 목표 레벨 이름. */
	FName TargetLevelName = NAME_None;

	/** @brief 전이 맵(로딩 맵) 이름. */
	FName LoadingMapName = NAME_None;

	/** @brief 미리 로드할 에셋 목록 캐싱. */
	TArray<TSoftObjectPtr<UObject>> PendingAssetsToLoad;

	/** @brief 로딩 시작 후 누적 시간. */
	float TotalElapsedTime = 0.0f;

	/** @brief 최소 로딩 보장 시간 (초). */
	const float MinLoadingTime = 2.0f;

	/** @brief 현재 로딩 시퀀스가 진행 중인지 여부. */
	bool bIsLoadingInProgress = false;

	/** @brief 현재 로딩에 사용할 커스텀 배경 이미지 캐싱 */
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> PendingLoadingImage = nullptr;
#pragma endregion 데이터 및 상태
};