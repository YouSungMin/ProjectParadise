// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsPopupWidget.generated.h"

#pragma region 전방 선언
class USlider;
class UParadiseCommonButton;
class USoundMix;
class USoundClass;
class ULevelLoadingSubsystem;
class UAudioSettingsSubsystem;
#pragma endregion 전방 선언

/**
 * @class USettingsPopupWidget
 * @brief 인게임 설정 팝업 위젯입니다.
 * @details 생성 시 게임을 일시정지하고 UI 입력 모드로 전환합니다.
 *          사운드 볼륨 제어, 게임 재개, 로비 귀환 기능을 제공합니다.
 *          슬라이더 드래그 시 RAM만 변경하고, 팝업이 닫힐 때 디스크에 1회 저장합니다.
 */
UCLASS()
class PARADISE_API USettingsPopupWidget : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region 생명주기
protected:
	/**
	 * @brief 위젯이 최초 생성될 때 호출됩니다.
	 * @details 서브시스템 캐싱, 델리게이트 바인딩, 슬라이더 초기값 세팅을 1회 수행합니다.
	 */
	virtual void NativeConstruct() override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 팝업을 화면에 표시하고 게임을 일시정지합니다.
	 * @details 인게임 HUD나 컨트롤러에서 설정 버튼을 눌렀을 때 호출해야 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void OpenSettings();

	/**
	 * @brief 팝업을 화면에서 숨기고 게임 시간을 복구하며 디스크 저장을 수행합니다.
	 * @details 내부 버튼이나 외부(ESC 키 등)에서 창을 닫을 때 호출합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|UI")
	void CloseSettings();
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/**
	 * @brief 슬라이더 초기값을 AudioSettingsSubsystem의 RAM 볼륨으로 세팅합니다.
	 * @details 서브시스템은 Initialize()에서 이미 디스크 로드를 완료한 상태입니다.
	 */
	void InitializeVolumeSliders();

	/**
	 * @brief BGM 슬라이더 값 변경 시 호출됩니다.
	 * @param Value 0.0 ~ 1.0 범위의 볼륨 값
	 * @details 서브시스템의 RAM만 변경하고, SetSoundMixClassOverride로 즉시 적용합니다.
	 *          디스크 저장은 NativeDestruct에서 1회만 수행합니다.
	 */
	UFUNCTION()
	void OnBGMVolumeChanged(float Value);

	/**
	 * @brief SFX 슬라이더 값 변경 시 호출됩니다.
	 * @param Value 0.0 ~ 1.0 범위의 볼륨 값
	 * @details 서브시스템의 RAM만 변경하고, SetSoundMixClassOverride로 즉시 적용합니다.
	 *          디스크 저장은 NativeDestruct에서 1회만 수행합니다.
	 */
	UFUNCTION()
	void OnSFXVolumeChanged(float Value);

	/**
	 * @brief 돌아가기 버튼 클릭 시 호출되어 팝업을 닫습니다.
	 * @details RemoveFromParent를 호출하면 NativeDestruct가 자동 실행되어 시간이 복구됩니다.
	 */
	UFUNCTION()
	void OnResumeGameClicked();

	/**
	 * @brief 로비로 가기 버튼 클릭 시 호출되어 씬을 전환합니다.
	 * @details LevelLoadingSubsystem을 통해 로딩 화면을 거쳐 로비 레벨로 이동합니다.
	 */
	UFUNCTION()
	void OnReturnToLobbyClicked();

	/**
	 * @brief 현재 스테이지를 재시작합니다.
	 * @details 현재 레벨 이름을 가져와 동일한 레벨을 다시 로드합니다.
	 */
	UFUNCTION()
	void OnRetryClicked();
#pragma endregion 내부 로직

#pragma region 위젯 바인딩
protected:
	/** @brief BGM 볼륨 조절 슬라이더 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_BGM = nullptr;

	/** @brief SFX 볼륨 조절 슬라이더 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_SFX = nullptr;

	/**
	 * @brief 닫기 (또는 게임으로 돌아가기) 버튼
	 * @note 모든 팝업에 필수이므로 BindWidget 유지
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UParadiseCommonButton> Btn_ResumeGame = nullptr;

	/**
	 * @brief 로비로 돌아가는 버튼
	 * @note BindWidgetOptional로 변경
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UParadiseCommonButton> Btn_ReturnToLobby = nullptr;

	/** @brief 현재 스테이지 재시작 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UParadiseCommonButton> Btn_Retry = nullptr;
#pragma endregion 위젯 바인딩

#pragma region 공통 UI 에셋 설정 (Config)
protected:
	/** * @brief 게임으로 돌아가기 버튼의 기본 이미지
	 * @details 눌림 효과는 UParadiseCommonButton의 틴트(bEnablePressedTint) 기능을 사용하므로 1장만 필요합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Config", meta = (DisplayName = "돌아가기 버튼 이미지"))
	TObjectPtr<UTexture2D> Tex_ResumeGame = nullptr;

	/** * @brief 로비로 돌아가기 버튼의 기본 이미지
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Config", meta = (DisplayName = "로비 귀환 버튼 이미지"))
	TObjectPtr<UTexture2D> Tex_ReturnToLobby = nullptr;

	/** @brief 다시하기 버튼 이미지 */
	UPROPERTY(EditDefaultsOnly, Category = "Paradise|UI|Config", meta = (DisplayName = "다시하기 버튼 이미지"))
	TObjectPtr<UTexture2D> Tex_Retry = nullptr;
#pragma endregion 공통 UI 에셋 설정 (Config)

#pragma region 데이터 드리븐 설정
protected:
	/**
	 * @brief 오디오 마스터 믹스.
	 * @details 기획자가 BP 디테일 패널에서 할당해야 볼륨 제어가 작동합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (DisplayName = "마스터 사운드 믹스"))
	TObjectPtr<USoundMix> MasterSoundMix = nullptr;

	/**
	 * @brief 배경음악(BGM) 사운드 클래스.
	 * @details 이 클래스에 속한 모든 사운드의 볼륨이 일괄 제어됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (DisplayName = "BGM 사운드 클래스"))
	TObjectPtr<USoundClass> BGMSoundClass = nullptr;

	/**
	 * @brief 효과음(SFX) 사운드 클래스.
	 * @details 이 클래스에 속한 모든 사운드의 볼륨이 일괄 제어됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (DisplayName = "SFX 사운드 클래스"))
	TObjectPtr<USoundClass> SFXSoundClass = nullptr;

	/**
	 * @brief 로비 레벨 이름.
	 * @details 로비로 돌아가기 버튼 클릭 시 이동할 레벨을 기획자가 설정합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Level", meta = (DisplayName = "로비 레벨 이름"))
	FName LobbyLevelName = FName("L_Lobby");

	/**
	 * @brief 로딩 전환 맵 이름.
	 * @details 레벨 전환 시 경유할 로딩 맵 이름을 기획자가 설정합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Level", meta = (DisplayName = "로딩 맵 이름"))
	FName LoadingMapName = FName("L_Loading");

	/**
	 * @brief 팝업이 열릴 때 게임 시간을 정지할지 여부입니다.
	 * @details 인게임에서는 true, 타이틀/로비에서는 false로 에디터에서 설정합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Config", meta = (DisplayName = "열릴 때 게임 일시정지"))
	bool bPauseGameOnOpen = true;
#pragma endregion 데이터 드리븐 설정

#pragma region 런타임 상태
private:
	/**
	 * @brief 캐싱된 AudioSettingsSubsystem.
	 * @details NativeConstruct에서 캐싱하여 매번 GetSubsystem 비용을 절약합니다.
	 */
	TWeakObjectPtr<UAudioSettingsSubsystem> CachedAudioSettings = nullptr;
#pragma endregion 런타임 상태
};
