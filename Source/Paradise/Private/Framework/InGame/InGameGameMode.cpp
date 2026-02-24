// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/InGame/InGameGameMode.h"
#include "Framework/InGame/InGameGameState.h"
#include "Framework/InGame/InGameController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/System/SquadSubsystem.h"
#include "Framework/System/EconomySubsystem.h"
#include "Framework/System/StageSubsystem.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Framework/InGame/Actors/DamageTextActor.h"

void AInGameGameMode::ForceVictory()
{
	SetGamePhase(EGamePhase::Victory);
}

AInGameGameMode::AInGameGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AInGameGameMode::BeginPlay()
{
	Super::BeginPlay();

	//GameState 캐싱
	CachedGameState = GetGameState<AInGameGameState>();


	FName TargetStageToPlay = NAME_None;


	//0223 김성현 - 스테이지 연결 초기화 로직 연결
	//서브시스템에서 아까 골랐던 스테이지 ID Get 
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UStageSubsystem* StageSys = GI->GetSubsystem<UStageSubsystem>())
		{
			TargetStageToPlay = StageSys->GetSelectedStageID();
		}
	}

	//ID 전달
	if (!TargetStageToPlay.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("🚀 [GameMode] 전달받은 스테이지 데이터(%s)로 게임을 세팅합니다!"), *TargetStageToPlay.ToString());
		FString DebugMsg = FString::Printf(TEXT("🎯 [인게임] 전달받기 성공! 현재 플레이할 스테이지: %s"), *TargetStageToPlay.ToString());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugMsg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, DebugMsg);
		InitializeStageData(TargetStageToPlay); 
	}
	else
	{
		// 에러 방지용 비상 코드
		UE_LOG(LogTemp, Error, TEXT("⚠️ [GameMode] 전달받은 스테이지 ID가 없습니다! 임시로 1-1을 실행합니다."));
		InitializeStageData(FName("Stage1_1"));
		FString ErrorMsg = TEXT("🚨 [인게임] 경고: 스테이지 ID를 전달받지 못했습니다! (None)");
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMsg);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, ErrorMsg);
	}

	//전투 시작 전, 데미지 텍스트 미리 넣기
	if (DamageTextClass)
	{
		if (UObjectPoolSubsystem* PoolSystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
		{
			//PoolSystem->PreSpawnPool(DamageTextClass, GetWorld(), PreSpawnDamageTextCount);
		}
	}

	//초기 상태 설정
	CurrentPhase = EGamePhase::Result;

	//게임 시작 상태 Ready로 설정
	SetGamePhase(EGamePhase::Combat);

	
}

void AInGameGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	SetupPlayerSquad(NewPlayer);
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

void AInGameGameMode::SetupPlayerSquad(APlayerController* NewPlayer)
{
	AInGameController* PC = Cast<AInGameController>(NewPlayer);
	if (!PC) return;

	AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>();
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	USquadSubsystem* SquadSys = GI ? GI->GetSubsystem<USquadSubsystem>() : nullptr;

	if (PS && SquadSys)
	{
		//서브시스템에서 로비에서 편성한 3인 스쿼드 배열 가져오기
		TArray<FName> MyPlayerIDs = SquadSys->GetPlayerSquad();

		// [방어 코드]
		// 배열이 비어있거나, 3칸 다 Name_None일 경우
		if (MyPlayerIDs.Num() == 0 || (MyPlayerIDs.IsValidIndex(0) && MyPlayerIDs[0].IsNone() && MyPlayerIDs[1].IsNone() && MyPlayerIDs[2].IsNone()))
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ 편성된 플레이어가 없습니다! 기본 캐릭터(테스트용)를 강제 스폰합니다."));

			MyPlayerIDs.Init(NAME_None, 3);
			MyPlayerIDs[0] = TEXT("test1");
		}

		//(PlayerData) 3개 스폰 및 인벤토리(장비) Init
		PS->InitSquad(MyPlayerIDs);

		//(PlayerBase) 3개 스폰 및 Init
		PC->InitializeSquadPawns();

		UE_LOG(LogTemp, Log, TEXT("✅ [%s] 플레이어의 스쿼드 세팅이 성공적으로 완료되었습니다."), *PC->GetName());
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
	//GameInstance 가져오기
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GameMode] GameInstance를 찾을 수 없습니다."));
		return;
	}

	//0220 김성현 - GI 데이터 테이블 검색 템플릿 함수 이용 로직 변경
	FStageStats* Row = GI->GetDataTableRow<FStageStats>(GI->StatgeStatsDataTable, StageID);
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



		//0223 김성현 - 클리어 재화 보상 적용 ,GI로 세이브 데이터 저장
		if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
		{
			if (UEconomySubsystem* EconomySys = GI->GetSubsystem<UEconomySubsystem>())
			{
				EconomySys->AddCurrency(ECurrencyType::Gold, CurrentStageData.ClearGold);
				EconomySys->AddCurrency(ECurrencyType::Aether, CurrentStageData.ClearAether);

				//1회성 클리어보상? //계속?
				//나머지 재화 추가
			}

			//경험치 보상 (전투에 참여한 스쿼드 멤버들에게 각각 추가)
			USquadSubsystem* SquadSys = GI->GetSubsystem<USquadSubsystem>();
			UInventorySystem* InvSys = GI->GetSubsystem<UInventorySystem>();
			if (SquadSys && InvSys) {
				UE_LOG(LogTemp, Log, TEXT("경험치 보상 : SquadSys InvSys존재"));
				// 현재 편성된 스쿼드 명단을 가져옴
				const TArray<FName>& CurrentSquad = SquadSys->GetPlayerSquad();

				for (const FName& HeroID : CurrentSquad)
				{
					if (!HeroID.IsNone())
					{
						// 각 캐릭터에게 클리어 경험치 지급!
						InvSys->AddCharacterExp(HeroID, CurrentStageData.ClearExp);
					}
				}
			}

			if (UStageSubsystem* StageSys = GI->GetSubsystem<UStageSubsystem>())
			{
				// 1. 다음 스테이지 해금
				StageSys->UnlockStage(CurrentStageData.NextStageID);
				
				// 2. 현재 스테이지 별점 기록 (임시로 3별 달성이라 가정, 추후 기획에 맞춰 수정)
				//StageSys->RecordStageClearStar(CurrentStageData., 3);
			}


			GI->SaveGameData();
		}
	}


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
	
	//GI로 세이브 데이터 저장
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		GI->SaveGameData();
	}
	
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

