// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingWidget.generated.h"

#pragma region 전방 선언
class UProgressBar;
class UTexture2D;
class UImage;
class UTextBlock;
class UWidgetAnimation;
class URetainerBox;
class UMaterialParameterCollection;
#pragma endregion 전방 선언

/**
 * @struct FSpecialLoadingImages
 * @brief 상황별 로딩 배경 이미지 세트 구조체
 */
USTRUCT(BlueprintType)
struct FSpecialLoadingImages
{
	GENERATED_BODY()

	/** @brief 타이틀에서 로비로 진입할 때 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	TSoftObjectPtr<UTexture2D> TitleToLobby = nullptr;

	/** @brief 인게임(스테이지)에서 로비로 복귀할 때 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	TSoftObjectPtr<UTexture2D> StageToLobby = nullptr;

	/** @brief 그 외 일반 상황 및 폴백 배경 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	TSoftObjectPtr<UTexture2D> DefaultBackground = nullptr;
};

/**
 * @class ULoadingWidget
 * @brief 비동기 로딩 진행률을 시각적으로 표시하는 위젯 클래스.
 * @details ParadiseGameInstance에서 계산된 로딩 퍼센트(0.0 ~ 1.0)를 받아 ProgressBar와 텍스트를 갱신합니다.
 */
UCLASS()
class PARADISE_API ULoadingWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	ULoadingWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

	/** @brief 애니메이션 종료 시점 감지 (Disappear 종료 시 로직 처리용) */
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;
#pragma region 외부 인터페이스
public:
	/**
	 * @brief 현재 맵과 목표 맵 정보를 바탕으로 최적의 배경 이미지를 초기화합니다.
	 * @param CurrentLevel 현재 맵 이름
	 * @param TargetLevel 목표 맵 이름
	 * @param InDefaultStageImage 스테이지 데이터 테이블에서 넘어온 로딩 이미지
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void InitLoadingImage(FName CurrentLevel, FName TargetLevel, TSoftObjectPtr<UTexture2D> InDefaultStageImage);

	/**
	 * @brief 로딩 진행률을 설정하고 UI를 갱신합니다.
	 * @param Percent 진행률 (0.0 ~ 1.0 범위)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void SetLoadingPercent(float Percent);

	/**
	 * @brief 로딩 팁이나 상태 메시지를 변경합니다.
	 * @param NewText 표시할 텍스트
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void SetLoadingText(FText InName, FText InDesc);

	/**
	 * @brief 로딩 배경 이미지를 교체합니다.
	 * @param InTexture 교체할 배경 텍스처 (nullptr이면 기본 설정 유지)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void SetBackgroundImage(UTexture2D* InTexture);

	/**
	* @brief Progress=1로 즉시 화면을 덮습니다. (L_Loading/목적지 레벨 플래시 방지)
	*/
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void InitAsCovered();

	/**
	 * @brief Anim_Disappear를 재생합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void PlayDisappearAnim();

	/** @brief MPC Progress를 0으로 리셋하여 내용물을 표시합니다. */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Loading")
	void ShowContent();
#pragma endregion 외부 인터페이스

#pragma region 이벤트 (블루프린트 확장)
protected:
	/** 
	 * @brief 로딩이 완료되었을 때(100%) 호출되는 이벤트. 
	 * @details 블루프린트에서 페이드 아웃 애니메이션 등을 구현 (일부러 BlueprintImplementableEvent 써서 기획자가 알아서 하도록 만들었음)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Paradise|UI|Loading")
	void OnLoadingComplete();

	/** @brief 기획자가 WBP_Loading 디테일 패널에서 직접 설정할 이미지 뭉치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paradise|Loading")
	FSpecialLoadingImages SpecialImages;
#pragma endregion 이벤트 (블루프린트 확장)

#pragma region 위젯 바인딩
private:
	/** @brief 스테이지 이름 표시용 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StageName = nullptr;

	/** @brief 스테이지 설명/팁 표시용 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StageDesc = nullptr;

	/** @brief 로딩 진행 바 (필수 바인딩) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_LoadingBar = nullptr;

	/** @brief 커스텀 로딩 배경 이미지 (선택 바인딩) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Background = nullptr;

	/** @brief 렌더링된 전체 UI에 머티리얼을 씌우는 최상위 리테이너 박스 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URetainerBox> Retainer_TransitionEffect = nullptr;

	/** @brief 화면을 덮는 연출 (MPC 제어 트랙 포함) */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_Appear = nullptr;

	/** @brief 화면을 여는 연출 (MPC 제어 트랙 포함) */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_Disappear = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 내부 상태 제어
private:
	/** @brief 사라지는 연출 중복 실행 방지 플래그 */
	bool bIsDisappearing = false;
#pragma endregion 내부 상태 제어

#pragma region 데이터 드리븐 설정
protected:
	/** @brief Progress 파라미터를 즉시 제어하기 위한 MPC 레퍼런스 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|Loading")
	TObjectPtr<UMaterialParameterCollection> MPC_Loading = nullptr;
#pragma endregion 데이터 드리븐 설정
};
