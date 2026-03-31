// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/InGame/InGameGameState.h"
#include "Objects/HomeBase.h"

/**
 * @details 상태 중복 체크 후 값을 갱신하며, 델리게이트에 바인딩된 UI 및 시스템에 상태 변화를 알립니다.
 * UI: 결과창 표시 및 타이머 활성화
 * 캐릭터: 페이즈에 따른 입력 제어(Ready/Result) 및 애니메이션 재생(Victory/Defeat)
 * 시스템: 몬스터 스폰 시작/중지 및 BGM 교체
 */
void AInGameGameState::SetCurrentPhase(EGamePhase NewPhase)
{
    // 1. 중복 호출 방지
    if (CurrentPhase == NewPhase) return;

    // 2. 상태 값 갱신
    CurrentPhase = NewPhase;

    // 3. 상태 변경 알림 (UI 및 기타 시스템)
    if (OnGamePhaseChanged.IsBound())
    {
        OnGamePhaseChanged.Broadcast(CurrentPhase);
    }

}

void AInGameGameState::RegisterAllyHomeBase(AHomeBase* InBase)
{
    AllyHomeBase = InBase;
    OnHomeBaseRegistered.Broadcast(true);
}

void AInGameGameState::RegisterEnemyHomeBase(AHomeBase* InBase)
{
    EnemyHomeBase = InBase;
    OnHomeBaseRegistered.Broadcast(false);
}