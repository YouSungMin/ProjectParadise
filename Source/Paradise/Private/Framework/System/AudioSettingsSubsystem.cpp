// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/AudioSettingsSubsystem.h"
#include "Framework/System/SettingsSaveGame.h"
#include "Kismet/GameplayStatics.h"

#pragma region 생명주기
void UAudioSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	/** @section 디스크에서 저장된 볼륨 로드 */
	LoadFromSlot();

	//UE_LOG(LogTemp, Log, TEXT("[AudioSettingsSubsystem] 초기화 완료. BGM=%.2f, SFX=%.2f"), CurrentBGMVolume, CurrentSFXVolume);
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 - Getter/Setter (RAM)
void UAudioSettingsSubsystem::SetBGMVolume(float NewVolume)
{
	CurrentBGMVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	//UE_LOG(LogTemp, Verbose, TEXT("[AudioSettings] BGM 볼륨 RAM 변경: %.2f"), CurrentBGMVolume);
}

void UAudioSettingsSubsystem::SetSFXVolume(float NewVolume)
{
	CurrentSFXVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	//UE_LOG(LogTemp, Verbose, TEXT("[AudioSettings] SFX 볼륨 RAM 변경: %.2f"), CurrentSFXVolume);
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
	if (UGameplayStatics::SaveGameToSlot(SaveObj, SaveSlotName, 0))
	{
		UE_LOG(LogTemp, Log, TEXT("[AudioSettings] 디스크 저장 완료! (슬롯: %s, BGM=%.2f, SFX=%.2f)"),
			*SaveSlotName, CurrentBGMVolume, CurrentSFXVolume);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[AudioSettings] 디스크 저장 실패!"));
	}
}

void UAudioSettingsSubsystem::LoadFromSlot()
{
	/** @section 1. 세이브 파일 존재 여부 확인 */
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioSettings] 세이브 파일 없음. 기본값 사용 (BGM=%.2f, SFX=%.2f)"),
			DefaultBGMVolume, DefaultSFXVolume);

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
		UE_LOG(LogTemp, Error, TEXT("[AudioSettings] 세이브 파일 로드 실패! 기본값 사용"));
		CurrentBGMVolume = DefaultBGMVolume;
		CurrentSFXVolume = DefaultSFXVolume;
		return;
	}

	/** @section 3. RAM에 로드된 값 복사 */
	CurrentBGMVolume = LoadObj->BGMVolume;
	CurrentSFXVolume = LoadObj->SFXVolume;

	UE_LOG(LogTemp, Log, TEXT("[AudioSettings] 디스크 로드 완료! (BGM=%.2f, SFX=%.2f)"),
		CurrentBGMVolume, CurrentSFXVolume);
}
#pragma endregion 외부 인터페이스 - 디스크 I/O (1회)
