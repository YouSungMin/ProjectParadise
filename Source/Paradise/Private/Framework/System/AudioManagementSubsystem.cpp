// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/AudioManagementSubsystem.h"
#include "Framework/System/AudioSettingsSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

void UAudioManagementSubsystem::PlayBGM(USoundBase* NewBGM, bool bLoop, float FadeTime)
{
	if (!NewBGM) return;

	if (UAudioSettingsSubsystem* AudioSettings = GetGameInstance()->GetSubsystem<UAudioSettingsSubsystem>())
	{
		AudioSettings->ApplyVolumeSettings();
	}

	// 이미 같은 음악이 재생 중이라면 중복 재생 방지
	if (ActiveBGMComponent && ActiveBGMComponent->GetSound() == NewBGM && ActiveBGMComponent->IsPlaying()) return;

	// 기존 음악 정지
	StopBGM(FadeTime);

	// 새 음악 스폰 (GameInstance를 Outer로 설정하여 레벨 전환 시에도 유지 가능성 확보)
	ActiveBGMComponent = UGameplayStatics::SpawnSound2D(GetWorld(), NewBGM, 1.0f, 1.0f, 0.0f, nullptr, true, true);

	if (ActiveBGMComponent)
	{
		ActiveBGMComponent->FadeIn(FadeTime);
	}
}

void UAudioManagementSubsystem::StopBGM(float FadeTime)
{
	if (ActiveBGMComponent && ActiveBGMComponent->IsPlaying())
	{
		ActiveBGMComponent->FadeOut(FadeTime, 0.0f);
		ActiveBGMComponent = nullptr;
	}
}
