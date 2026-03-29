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

UENUM()
enum class ELoadingPhase : uint8
{
	None,
	Appearing,    // 현재 레벨에서 Anim_Appear 재생 중
	Loading,      // L_Loading에서 프로그레스바 진행 중
	Disappearing  // 목적지 레벨에서 Anim_Disappear 재생 중
};

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
	 * @brief 레벨 전이 시작
	 * @param InTargetLevelName 목표 레벨
	 * @param InLoadingMapName 로딩 맵 이름
	 * @param InAssetsToPreload 프리로드 에셋
	 * @param InLoadingImage 데이터테이블용 로딩 이미지
	 * @param InStageName 스테이지 이름 (로딩창 표시용)
	 * @param InStageDesc 스테이지 설명 (로딩창 표시용)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|System|Loading", meta = (AutoCreateRefTerm = "InAssetsToPreload, InLoadingMapName"))
	void StartLevelTransition(
		FName InTargetLevelName,
		FName InLoadingMapName,
		const TArray<TSoftObjectPtr<UObject>>& InAssetsToPreload,
		TSoftObjectPtr<UTexture2D> InLoadingImage = nullptr,
		FText InStageName = FText::GetEmpty(), 
		FText InStageDesc = FText::GetEmpty()  
	);

	/** @brief 로딩 위젯 클래스 설정 (GameInstance 호출용) */
	void SetLoadingWidgetClass(TSubclassOf<UUserWidget> NewLoadingWidgetClass);

	/**
	 * @brief Anim_Disappear 종료 후 LoadingWidget에서 호출합니다.
	 * @details 최종 레벨로 이동합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|System|Loading")
	void ExecuteFinalTransition();
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

	/**
	 * @brief 현재 플레이어의 편대(캐릭터, 소환수) 및 장착 장비 정보를 바탕으로 로드할 에셋 경로를 동적으로 수집합니다.
	 * @param OutAssetPaths 수집된 에셋 경로를 담을 배열 (참조 전달)
	 */
	void GatherDynamicAssetsToLoad(TArray<FSoftObjectPath>& OutAssetPaths);
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

	/** @brief 전이 시작 전의 실제 출발지 레벨 이름 (예: L_Title, L_Stage1) */
	FName OriginLevelName = NAME_None;

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

	/** @brief 가짜 로딩(시간 기반)이 도달할 수 있는 최대 퍼센트 (30%) */
	const float MaxFakePercent = 0.7f;

	/** @brief 현재 로딩 시퀀스가 진행 중인지 여부. */
	bool bIsLoadingInProgress = false;

	/** @brief 현재 로딩에 사용할 커스텀 배경 이미지 캐싱 */
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> PendingLoadingImage = nullptr;

	/** @brief 로딩 위젯에 표시할 텍스트 캐싱 */
	FText PendingStageName;
	FText PendingStageDesc;
#pragma endregion 데이터 및 상태

#pragma region 내부 상태 추가
private:
	ELoadingPhase CurrentPhase = ELoadingPhase::None;
#pragma endregion 내부 상태 추가
	public:
		/** @brief 위젯의 Anim_Appear 완료 시 호출 */
		void NotifyAppearFinished();

		/** @brief 위젯의 Anim_Disappear 완료 시 호출 */
		void NotifyDisappearFinished();

		/** @brief 현재 Appearing 단계인지 확인 */
		bool IsAppearingPhase() const { return CurrentPhase == ELoadingPhase::Appearing; }
};