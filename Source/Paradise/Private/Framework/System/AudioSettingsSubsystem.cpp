// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/AudioSettingsSubsystem.h"
#include "Framework/System/SettingsSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"

#pragma region 생명주기
void UAudioSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	/** @section 디스크에서 저장된 볼륨 로드 */
	LoadFromSlot();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UAudioSettingsSubsystem::OnMapLoaded);
}
void UAudioSettingsSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	Super::Deinitialize();
}

void UAudioSettingsSubsystem::OnMapLoaded(UWorld* LoadedWorld)
{
	ApplyVolumeSettings();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 - Getter/Setter (RAM)
void UAudioSettingsSubsystem::SetBGMVolume(float NewVolume)
{
	CurrentBGMVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
}

void UAudioSettingsSubsystem::SetSFXVolume(float NewVolume)
{
	CurrentSFXVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
}

void UAudioSettingsSubsystem::ApplyVolumeSettings()
{
	if (!MasterSoundMix) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// SM_Master 활성화
	UGameplayStatics::PushSoundMixModifier(World, MasterSoundMix);

	// 저장된 볼륨 즉시 적용
	if (BGMSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			World, MasterSoundMix, BGMSoundClass,
			CurrentBGMVolume, 1.0f, 0.0f, true);
	}

	if (SFXSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			World, MasterSoundMix, SFXSoundClass,
			CurrentSFXVolume, 1.0f, 0.0f, true);
	}
}
#pragma endregion 외부 인터페이스 - Getter/Setter (RAM)

#pragma region 외부 인터페이스 - 디스크 I/O (1회)
void UAudioSettingsSubsystem::SaveToSlot()
{
	/** @section 1. 세이브 객체 생성 */
	USettingsSaveGame* SaveObj = Cast<USettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(USettingsSaveGame::StaticClass())
	);

	if (!SaveObj)
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [AudioSettings] 세이브 객체 생성 실패!"));
		return;
	}

	/** @section 2. RAM 데이터를 세이브 객체로 복사 */
	SaveObj->BGMVolume = CurrentBGMVolume;
	SaveObj->SFXVolume = CurrentSFXVolume;

	/** @section 3. 디스크에 쓰기 (I/O 발생) */
	UGameplayStatics::SaveGameToSlot(SaveObj, SaveSlotName, 0);
}

void UAudioSettingsSubsystem::LoadFromSlot()
{
	/** @section 1. 세이브 파일 존재 여부 확인 */
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		/*UE_LOG(LogTemp, Warning, TEXT("[AudioSettings] 세이브 파일 없음. 기본값 사용 (BGM=%.2f, SFX=%.2f)"),
			DefaultBGMVolume, DefaultSFXVolume);*/

		/** @section Fallback: 기본값 사용 */
		CurrentBGMVolume = DefaultBGMVolume;
		CurrentSFXVolume = DefaultSFXVolume;
		return;
	}

	/** @section 2. 디스크에서 읽기 (I/O 발생) */
	USettingsSaveGame* LoadObj = Cast<USettingsSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)
	);

	if (!LoadObj)
	{
		//UE_LOG(LogTemp, Error, TEXT("[AudioSettings] 세이브 파일 로드 실패! 기본값 사용"));
		CurrentBGMVolume = DefaultBGMVolume;
		CurrentSFXVolume = DefaultSFXVolume;
		return;
	}

	/** @section 3. RAM에 로드된 값 복사 */
	CurrentBGMVolume = LoadObj->BGMVolume;
	CurrentSFXVolume = LoadObj->SFXVolume;

	/*UE_LOG(LogTemp, Log, TEXT("[AudioSettings] 디스크 로드 완료! (BGM=%.2f, SFX=%.2f)"),
		CurrentBGMVolume, CurrentSFXVolume);*/
}
#pragma endregion 외부 인터페이스 - 디스크 I/O (1회)
