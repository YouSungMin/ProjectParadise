// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AudioSettingsSubsystem.generated.h"

#pragma region 전방 선언
class USettingsSaveGame;
class USoundMix;
class USoundClass;
#pragma endregion 전방 선언

/**
 * @class UAudioSettingsSubsystem
 * @brief 오디오 설정을 RAM에서 관리하고 디스크 저장/로드를 담당하는 서브시스템입니다.
 * @details GameInstance의 수정 없이 OCP를 준수하며,
 *          슬라이더 드래그 시 RAM만 변경하고 팝업 닫힐 때 디스크에 1회 저장합니다.
 */
UCLASS()
class PARADISE_API UAudioSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
#pragma region 생명주기
public:
	/**
	 * @brief 서브시스템 초기화 시 저장된 볼륨을 디스크에서 로드합니다.
	 * @details GameInstance::Init() 이후 자동으로 호출됩니다.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	/** @brief 맵 로드 완료 시 볼륨 자동 적용 */
	void OnMapLoaded(UWorld* LoadedWorld);
#pragma endregion 생명주기

#pragma region 외부 인터페이스 - Getter/Setter (RAM)
public:
	/**
	 * @brief 현재 BGM 볼륨을 가져옵니다 (RAM).
	 * @return 0.0 ~ 1.0 범위의 볼륨 값
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	float GetBGMVolume() const { return CurrentBGMVolume; }

	/**
	 * @brief 현재 SFX 볼륨을 가져옵니다 (RAM).
	 * @return 0.0 ~ 1.0 범위의 볼륨 값
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	float GetSFXVolume() const { return CurrentSFXVolume; }

	/**
	 * @brief BGM 볼륨을 RAM에 설정합니다 (디스크 저장 안 함).
	 * @param NewVolume 0.0 ~ 1.0 범위의 새 볼륨 값
	 * @details 슬라이더 드래그 시 호출되며, 디스크 I/O는 발생하지 않습니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	void SetBGMVolume(float NewVolume);

	/**
	 * @brief SFX 볼륨을 RAM에 설정합니다 (디스크 저장 안 함).
	 * @param NewVolume 0.0 ~ 1.0 범위의 새 볼륨 값
	 * @details 슬라이더 드래그 시 호출되며, 디스크 I/O는 발생하지 않습니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	void SetSFXVolume(float NewVolume);

	/**
	 * @brief 현재 RAM의 볼륨 값을 사운드 시스템에 즉시 적용합니다.
	 * @details 게임 시작 시 저장된 볼륨을 복원할 때 사용합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	void ApplyVolumeSettings();

	/** @brief SoundMix 참조 (GameInstance에서 주입됨) */
	UPROPERTY()
	TObjectPtr<USoundMix> MasterSoundMix = nullptr;

	/** @brief BGM 사운드 클래스 참조 (GameInstance에서 주입됨) */
	UPROPERTY()
	TObjectPtr<USoundClass> BGMSoundClass = nullptr;

	/** @brief SFX 사운드 클래스 참조 (GameInstance에서 주입됨) */
	UPROPERTY()
	TObjectPtr<USoundClass> SFXSoundClass = nullptr;
#pragma endregion 외부 인터페이스 - Getter/Setter (RAM)

#pragma region 외부 인터페이스 - 디스크 I/O (1회)
public:
	/**
	 * @brief 현재 RAM에 있는 볼륨 값을 디스크에 저장합니다.
	 * @details 설정 팝업이 닫힐 때(NativeDestruct) 딱 1회만 호출되어야 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	void SaveToSlot();

	/**
	 * @brief 디스크에서 저장된 볼륨 값을 RAM으로 로드합니다.
	 * @details Initialize()에서 자동 호출되며, 수동 호출도 가능합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Audio")
	void LoadFromSlot();
#pragma endregion 외부 인터페이스 - 디스크 I/O (1회)

#pragma region 내부 데이터
private:
	/** @brief 세이브 슬롯 이름 (하드코딩 방지 및 변경 불가 상태로 고정) */
	const FString SaveSlotName = TEXT("AudioSettings");

	/** @brief BGM 기본 볼륨 (0.0 ~ 1.0) */
	float DefaultBGMVolume = 0.7f;

	/** @brief SFX 기본 볼륨 (0.0 ~ 1.0) */
	float DefaultSFXVolume = 0.8f;

	/** @brief 현재 BGM 볼륨 (RAM) */
	float CurrentBGMVolume = 0.7f;

	/** @brief 현재 SFX 볼륨 (RAM) */
	float CurrentSFXVolume = 0.8f;
#pragma endregion 내부 데이터
};
