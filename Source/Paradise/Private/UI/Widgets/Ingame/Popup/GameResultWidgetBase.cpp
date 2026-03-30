// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/Popup/GameResultWidgetBase.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/System/LevelLoadingSubsystem.h" // 로딩 시스템 사용
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Assets/ParadiseFXAudioData.h"

void UGameResultWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

#pragma region 이벤트 바인딩
	if (Btn_Lobby) Btn_Lobby->OnClicked.AddUniqueDynamic(this, &UGameResultWidgetBase::OnLobbyClicked);
	if (Btn_Retry) Btn_Retry->OnClicked.AddUniqueDynamic(this, &UGameResultWidgetBase::OnRetryClicked);
#pragma endregion 이벤트 바인딩
}

#pragma region 내부 로직 구현
void UGameResultWidgetBase::OnLobbyClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
	}

	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_IngameReturnToLobby)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_IngameReturnToLobby);
		}
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_Lobby,
			this,
			&UGameResultWidgetBase::ExecuteLobby,
			0.3f,
			false
		);
	}
}

void UGameResultWidgetBase::OnRetryClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
	}

	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_IngameRetry)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_IngameRetry);
		}
	}

	UWorld* World = GetWorld();
	//UE_LOG(LogTemp, Error, TEXT("[ResultBase] GetWorld(): %s"), World ? TEXT("유효") : TEXT("NULL"));

	if (World)
	{
		World->GetTimerManager().SetTimer(
			TimerHandle_Retry,
			this,
			&UGameResultWidgetBase::ExecuteRetry,
			0.3f,
			false
		);
	}
}

void UGameResultWidgetBase::ExecuteLobby()
{
	// 서브시스템을 통해 로비로 이동
	if (ULevelLoadingSubsystem* LoadingSystem = GetGameInstance()->GetSubsystem<ULevelLoadingSubsystem>())
	{
		// 로딩 맵(L_Loading)을 경유하여 로비(L_Lobby)로 이동
		// 에셋 프리로딩이 필요 없다면 빈 배열 전달
		TArray<TSoftObjectPtr<UObject>> EmptyAssets;
		LoadingSystem->StartLevelTransition(FName("L_Lobby"), FName("L_Loading"), EmptyAssets);
	}
	else
	{
		// 예외 처리: 서브시스템 실패 시 직접 이동s
		UGameplayStatics::OpenLevel(this, FName("L_Lobby"));
	}
}

void UGameResultWidgetBase::ExecuteRetry()
{
	//UE_LOG(LogTemp, Error, TEXT("[ResultBase] ExecuteRetry 호출됨!"));
	// 현재 레벨 이름 가져와서 다시 열기
	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);

	// 재시작은 로딩창 없이 바로 여는 경우가 많지만, 필요하면 서브시스템 사용 가능
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
}
#pragma endregion 내부 로직 구현