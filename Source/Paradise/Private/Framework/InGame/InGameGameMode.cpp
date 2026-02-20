// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/InGame/InGameGameMode.h"
#include "Framework/InGame/InGameGameState.h"
#include "Framework/Core/ParadiseGameInstance.h"


AInGameGameMode::AInGameGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AInGameGameMode::BeginPlay()
{
	Super::BeginPlay();

	//GameState 캐싱
	CachedGameState = GetGameState<AInGameGameState>();

	//임시로 1-1 스테이지 정보로 초기화 -> (나중에 GameInstance 연동)
	InitializeStageData(FName("Stage1_1"));

	//초기 상태 설정
	CurrentPhase = EGamePhase::Result;

	//게임 시작 상태 Ready로 설정
	SetGamePhase(EGamePhase::Combat);

}

void AInGameGameMode::OnStageTimerElapsed()
{
	if (!CachedGameState) return;

	CachedGameState->RemainingTime -= 1.0f;

	// [디버깅용] 로그로 타이머 확인
	//UE_LOG(LogTemp, Log, TEXT("Time: %.0f"), CachedGameState->RemainingTime);

	//타이머 변경 델리게이트 브로드캐스트
	if (CachedGameState->OnTimerChanged.IsBound())
	{
		int32 IntTime = FMath::CeilToInt(CachedGameState->RemainingTime);
		CachedGameState->OnTimerChanged.Broadcast(IntTime);
	}


	//시간 종료 체크
	if (CachedGameState->RemainingTime <= 0.f)
	{
		CachedGameState->RemainingTime = 0.f;
		CachedGameState->bIsTimerActive = false;

		//타임오버 패배 처리 -> EndStage 호출
		EndStage(false);
		UE_LOG(LogTemp, Warning, TEXT("시간 초과! 패배 처리 로직 실행"));
	}
}

/**
 * @details 페이즈를 변경하고, GameState에 전파하여 UI/캐릭터 등이 반응하게 합니다.
 * 이후 switch문을 통해 각 단계별 초기화 함수(OnPhase::)를 호출합니다.
 */
void AInGameGameMode::SetGamePhase(EGamePhase NewPhase)
{
	if (CurrentPhase == NewPhase) return;

	//상태 변경
	CurrentPhase = NewPhase;

	//게임스테이트에도 반영, Broadcast 발생
	if (CachedGameState) CachedGameState->SetCurrentPhase(CurrentPhase);

	//상태별 진입 로직
	switch (CurrentPhase)
	{
	case EGamePhase::Ready:
		OnPhaseReady();
		break;
	case EGamePhase::Combat:
		OnPhaseCombat();
		break;
	case EGamePhase::Victory:
		OnPhaseVictory();
		break;
	case EGamePhase::Defeat:
		OnPhaseDefeat();
		break;
	case EGamePhase::Result:
		OnPhaseResult();
		break;
	}
}

void AInGameGameMode::EndStage(bool bIsVictory)
{
	//이미 종료된 상태면 무시
	if (CurrentPhase == EGamePhase::Victory || CurrentPhase == EGamePhase::Defeat) return;

	//승리/패배 상태로 전환
	if (bIsVictory)
	{
		SetGamePhase(EGamePhase::Victory);
	}
	else
	{
		SetGamePhase(EGamePhase::Defeat);
	}
}

void AInGameGameMode::InitializeStageData(FName StageID)
{
	//0. GameInstance 가져오기
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GameMode] GameInstance를 찾을 수 없습니다."));
		return;
	}

	//1.GameInstance에 있는 스테이지 테이블 가져오기
	UDataTable* StageTable = GI->StatgeStatsDataTable; 
	if (!StageTable)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GameMode] 스테이지 데이터 테이블이 없습니다."));
		return;
	}

	//2. 데이터 테이블에서 정보 찾기
	FStageStats* Row = StageTable->FindRow<FStageStats>(StageID, TEXT("StageInfoContext"));
	if (Row)
	{
		CurrentStageData = *Row;	//데이터를 개별변수가 아닌 구조체에 한번에 복사

		//2. 게임스태이트에 정보 할당
		if (CachedGameState)
		{
			CachedGameState->DisplayStageName = CurrentStageData.StageName.ToString();
			CachedGameState->RemainingTime = CurrentStageData.TimeLimit;
			CachedGameState->bIsTimerActive = false;
		}

		//UE_LOG(LogTemp, Log, TEXT("스테이지 로그 완료: %s"), *CachedGameState->DisplayStageName);

		// [로그 추가] 스테이지 기본 정보 출력
		//UE_LOG(LogTemp, Warning, TEXT("========================================="));
		//UE_LOG(LogTemp, Warning, TEXT("[Stage Data Loaded] ID: %s"), *StageID.ToString());
		//UE_LOG(LogTemp, Log, TEXT(" - Name      : %s"), *CurrentStageData.StageName.ToString());
		//UE_LOG(LogTemp, Log, TEXT(" - Desc      : %s"), *CurrentStageData.Description.ToString());
		//UE_LOG(LogTemp, Log, TEXT(" - TimeLimit : %.1f sec"), CurrentStageData.TimeLimit);
		//UE_LOG(LogTemp, Warning, TEXT("========================================="));
	}
	else
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [GameMode] 스테이지 ID를 찾을 수 없습니다: %s"), *StageID.ToString());
	}
}

void AInGameGameMode::OnPhaseReady()
{
	UE_LOG(LogTemp, Log, TEXT("Phase: Ready (3초 카운트다운)"));

	//1. 플레이어 준비 상태로 전환

	//2. 3초 후 전투 단계(Combat)로 전환
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			SetGamePhase(EGamePhase::Combat);
		}, 3.0f, false);
}


void AInGameGameMode::OnPhaseCombat()
{
	UE_LOG(LogTemp, Log, TEXT("Phase: Combat (전투 시작)"));

	//1. 몬스터 스폰 매니저어게 웨이브 시작 요청하기 

	// 2. FTimer 가동, 타이머 시작 (1초마다, 반복해서, OnStageTimerElapsed 실행)
	GetWorldTimerManager().SetTimer(StageTimerHandle, this, &AInGameGameMode::OnStageTimerElapsed, 1.0f, true);
	if (CachedGameState) CachedGameState->bIsTimerActive = true;
}


void AInGameGameMode::OnPhaseVictory()
{
	// 승리했을 떄 타이머 정지 
	GetWorldTimerManager().ClearTimer(StageTimerHandle);

	UE_LOG(LogTemp, Log, TEXT("Phase: Victory! 보상 지급 준비"));
	if (CachedGameState)
	{
		//1. 타이머 정지(GameState의 플래그 사용)
		CachedGameState->bIsTimerActive = false;
		
		//보상 정보 캐싱
		CachedGameState->AcquiredGold = CurrentStageData.ClearGold;
		CachedGameState->AcquiredExp = CurrentStageData.ClearExp;

		//다음 스테이지 ID 캐싱
		CachedGameState->NextStageID = CurrentStageData.NextStageID;
	}

	// [로그 추가] 보상 및 다음 스테이지 정보 출력
	//UE_LOG(LogTemp, Warning, TEXT("============= [VICTORY] ============="));
	//UE_LOG(LogTemp, Log, TEXT(" $$$ Reward Gold : %d G"), CurrentStageData.ClearGold);
	//UE_LOG(LogTemp, Log, TEXT(" +++ Reward Exp  : %d XP"), CurrentStageData.ClearExp);
	//UE_LOG(LogTemp, Log, TEXT(" >>> Next Stage  : %s"), *CurrentStageData.NextStageID.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("====================================="));

	//2. 데이터 저장 (GameInstance 연동 필요)
	// TODO: GameInstance->AddGold(CurrentStageData.ClearGold);
	// TODO: GameInstance->UnlockStage(CurrentStageData.NextStageID);


	//3. 3초 후 결과 단계(Result)로 전환
	FTimerHandle ResultTimer;
	GetWorldTimerManager().SetTimer(ResultTimer, [this]() {
		SetGamePhase(EGamePhase::Result);
		}, 3.0f, false);
}

void AInGameGameMode::OnPhaseDefeat()
{
	GetWorldTimerManager().ClearTimer(StageTimerHandle);

	if(CachedGameState) CachedGameState->bIsTimerActive = false;
	UE_LOG(LogTemp, Error, TEXT("Phase: Defeat.... 보상없음"));

	//패배시 보상 없음
	
	//3초 후 결과 단계(Result)로 전환
	FTimerHandle ResultTimer;
	GetWorldTimerManager().SetTimer(ResultTimer, [this]() {
		SetGamePhase(EGamePhase::Result);
		}, 3.0f, false);
	
}

void AInGameGameMode::OnPhaseResult()
{
	//UE_LOG(LogTemp, Log, TEXT("Phase: Result (최종 결과 화면 출력)"));

	//버튼생성 후 로비로 레벨이동
}

