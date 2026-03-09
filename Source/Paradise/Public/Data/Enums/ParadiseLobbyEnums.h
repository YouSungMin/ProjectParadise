#pragma once

#include "CoreMinimal.h"

/**
 * @enum EParadiseLobbyMenu
 * @brief 로비에서 진입 가능한 메뉴 목록.
 * @details 'Home' 버튼은 제거하고, 아무것도 안 띄운 상태를 None으로 정의.
 */
UENUM(BlueprintType)
enum class EParadiseLobbyMenu : uint8
{
	None        UMETA(DisplayName = "로비"),       // 기본 로비 상태 (3D 뷰)
	Battle      UMETA(DisplayName = "전투"),       // 전투 출정
	StageMap    UMETA(DisplayName = "스테이지맵"), // 챕터 선택
	Summon      UMETA(DisplayName = "소환"),       // 소환
	Squad       UMETA(DisplayName = "편성"),       // 편성
	Enhance     UMETA(DisplayName = "강화"),		   // 강화
	Codex       UMETA(DisplayName = "도감")        // 도감
};