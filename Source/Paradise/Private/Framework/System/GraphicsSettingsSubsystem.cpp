// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/GraphicsSettingsSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "GenericPlatform/GenericPlatformMemory.h"

void UGraphicsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 엔진이 시작될 때 기존에 유저가 디스크(GameUserSettings.ini)에 저장해둔 세팅을 불러와 즉시 적용합니다.
	if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
	{
		UserSettings->LoadSettings(false);
		UserSettings->ApplySettings(false);
		UE_LOG(LogTemp, Log, TEXT("✅ [GraphicsSettings] 저장된 그래픽 설정을 로드했습니다. 현재 레벨: %d"), UserSettings->GetVisualEffectQuality());
	}
}

int32 UGraphicsSettingsSubsystem::GetGraphicsQuality() const
{
	if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
	{
		// 여러 옵션 중 '이펙트 퀄리티'를 기준으로 현재 단계를 반환
		return UserSettings->GetVisualEffectQuality();
	}
	return 2; // UserSettings를 못 가져올 경우의 기본값 (높음)
}

void UGraphicsSettingsSubsystem::SetGraphicsQuality(int32 NewQuality)
{
	if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
	{
		// 0(낮음) ~ 3(에픽) 사이의 값으로 안전하게 제한
		int32 ClampedQuality = FMath::Clamp(NewQuality, 0, 3);

		//그래픽 관련 옵션 조절
		UserSettings->SetVisualEffectQuality(ClampedQuality);
		UserSettings->SetPostProcessingQuality(ClampedQuality);
		UserSettings->SetShadowQuality(ClampedQuality); 
		UserSettings->SetTextureQuality(ClampedQuality); 

		// 변경 사항을 화면에 즉시 적용, 디스크 파일에 영구 저장
		UserSettings->ApplySettings(false);
		UserSettings->SaveSettings();

		//UE_LOG(LogTemp, Log, TEXT("🔧 [GraphicsSettings] 그래픽 퀄리티가 %d 단계로 변경 및 저장되었습니다."), ClampedQuality);
	}
}

void UGraphicsSettingsSubsystem::CheckDevicePerformanceAndApply()
{
	//현재 기기의 물리적 메모리 용량
	FPlatformMemoryConstants MemoryConstants = FPlatformMemory::GetConstants();

	// Byte 단위이므로 GB 단위로 변환
	float TotalRAM_GB = MemoryConstants.TotalPhysical / (1024.0f * 1024.0f * 1024.0f);

	UE_LOG(LogTemp, Log, TEXT("📱 [스펙 검사] 이 기기의 총 RAM 용량: %.2f GB"), TotalRAM_GB);

	//RAM이 4GB 이하라면 저사양 기기로 판정
	if (TotalRAM_GB <= 4.1f)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [스펙 검사] 저사양 기기 감지! 강제로 그래픽을 '낮음'으로 설정합니다."));

		// 강제로 제일 낮은 옵션(0)으로 맞춰버리고 디스크에 저장
		SetGraphicsQuality(0);

	}
}