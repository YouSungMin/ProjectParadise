// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ParadiseFXAudioData.generated.h"

#pragma region 전방 선언
class USoundBase;
#pragma endregion 전방 선언

/**
 * @class UParadiseFXAudioData
 * @brief 게임 전체(타이틀, 로비, 가챠, 인게임 등)의 UI 및 연출 사운드를 중앙 집중 관리하는 데이터 에셋입니다.
 * @details SRP(단일 책임 원칙)에 따라 오디오 기획자는 이 에셋 하나만 수정하여 게임 전반의 사운드를 제어합니다.
 */
UCLASS()
class PARADISE_API UParadiseFXAudioData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

#pragma region 공통 사운드 (Common UI)
	// 기획 요구사항: "타이틀 설정 클릭 = 로비 설정 클릭", "챕터 카메라 = 가챠 카메라", "각종 뒤로가기 동일"
	// 이렇게 중복되는 사운드는 공통으로 묶어서 메모리와 작업 동선을 최적화합니다.

	/** @brief [공통] 설정 팝업 버튼 클릭음 (타이틀, 로비, 인게임 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Common")
	TObjectPtr<USoundBase> SFX_SettingsOpen = nullptr;

	/** @brief [공통] 뒤로가기 버튼 클릭음 (설정 팝업, 스테이지, 가챠, 강화, 도감 등 모든 UI 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Common")
	TObjectPtr<USoundBase> SFX_CommonBack = nullptr;

	/** @brief [공통] 탭(Tab) 변경 클릭음 (소환 탭, 강화 탭, 도감 탭 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Common")
	TObjectPtr<USoundBase> SFX_CommonTabClick = nullptr;

	/** @brief [공통] 씬 전환 카메라 이동 연출음 (챕터 입장, 가챠 씬 진입 등 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Common")
	TObjectPtr<USoundBase> SFX_CameraMoveSwoosh = nullptr;

	/** @brief [공통] 재화 부족, 조건 미달 시 뜨는 경고/알림 팝업음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Common")
	TObjectPtr<USoundBase> SFX_ErrorOrNotEnough = nullptr;
#pragma endregion 공통 사운드 (Common UI)

#pragma region 타이틀 화면 (Title)
	/** @brief 타이틀 화면 배경음악 (BGM) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Title")
	TObjectPtr<USoundBase> BGM_Title = nullptr;

	/** @brief 'Touch to Start' 터치 시 재생될 진입 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Title")
	TObjectPtr<USoundBase> SFX_TouchToStart = nullptr;
#pragma endregion 타이틀 화면 (Title)

#pragma region 로비 화면 (Lobby)
	/** @brief 로비 화면 배경음악 (BGM) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Lobby")
	TObjectPtr<USoundBase> BGM_Lobby = nullptr;

	/** @brief 로비 메인 메뉴 5종 (에피소드, 소환, 편성, 강화, 도감) 터치 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Lobby")
	TObjectPtr<USoundBase> SFX_LobbyMenuClick = nullptr;
#pragma endregion 로비 화면 (Lobby)

#pragma region 에피소드 및 스테이지 (Stage)
	/** @brief 에피소드 챕터 버튼 터치 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Stage")
	TObjectPtr<USoundBase> SFX_ChapterClick = nullptr;

	/** @brief 스테이지 맵에서 노드(거점) 터치 시 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Stage")
	TObjectPtr<USoundBase> SFX_NodeClick = nullptr;

	/** @brief 스테이지 입장 전 UI (편성하기, 전투하기) 클릭음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Stage")
	TObjectPtr<USoundBase> SFX_StageActionClick = nullptr;
#pragma endregion 에피소드 및 스테이지 (Stage)

#pragma region 가챠 연출 (Summon)
	/** @brief 1회 뽑기 / 10회 뽑기 실행 버튼 클릭음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Summon")
	TObjectPtr<USoundBase> SFX_GachaPullExecute = nullptr;

	/** @brief 가챠 상자가 하늘에서 바닥으로 쿵! 떨어질 때 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Summon")
	TObjectPtr<USoundBase> SFX_GachaBoxDrop = nullptr;

	/** @brief 가챠 상자를 터치해서 뚜껑이 열리는 순간의 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Summon")
	TObjectPtr<USoundBase> SFX_GachaBoxTouchOpen = nullptr;

	/** @brief 구슬들이 하늘로 치솟았다가 바닥에 안착할 때 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Summon")
	TObjectPtr<USoundBase> SFX_GachaOrbDrop = nullptr;

	/** @brief 바닥에 놓인 구슬을 터치하여 파괴/리빌(Reveal)할 때 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Summon")
	TObjectPtr<USoundBase> SFX_GachaOrbReveal = nullptr;

	/** @brief 모든 연출이 끝나고 결과창(영수증) 확인 버튼 클릭 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Summon")
	TObjectPtr<USoundBase> SFX_GachaResultConfirm = nullptr;
#pragma endregion 가챠 연출 (Summon)

#pragma region 강화 및 성장 (Enhancement)
	/** @brief 강화 / 돌파 진행 버튼 클릭 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Enhance")
	TObjectPtr<USoundBase> SFX_EnhanceActionExecute = nullptr;

	/** @brief 강화 / 돌파 성공 시 화려하게 터지는 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Enhance")
	TObjectPtr<USoundBase> SFX_EnhanceSuccess = nullptr;
#pragma endregion 강화 및 성장 (Enhancement)

#pragma region 스쿼드 편성 (Formation)
	/** * @note 캐릭터 교체 음성(Voice)은 FCharacterAssets에서 관리하고,
	 * 무기/장비 장착 효과음은 UI의 Sound_WeaponEquipMap 에서 관리하도록 이미 분리되어 있습니다. (SRP 준수)
	 */

	 /** @brief 퍼밀리어(유닛)를 슬롯에 장착/교체할 때 효과음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Formation")
	TObjectPtr<USoundBase> SFX_FamiliarEquip = nullptr;
#pragma endregion 스쿼드 편성 (Formation)

#pragma region 인게임 및 결과 (In-Game)
	/** @brief 인게임 팝업에서 '로비로 가기' 버튼 터치음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|InGame")
	TObjectPtr<USoundBase> SFX_IngameReturnToLobby = nullptr;

	/** @brief 인게임 팝업에서 '다시 하기(Retry)' 버튼 터치음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|InGame")
	TObjectPtr<USoundBase> SFX_IngameRetry = nullptr;

	/** @brief 인게임 팝업에서 '다음으로(Next)' 버튼 터치음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|InGame")
	TObjectPtr<USoundBase> SFX_IngameNext = nullptr;

	/** @brief 전투 승리 시 결과창 팝업 효과음 (팡빠레 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|InGame")
	TObjectPtr<USoundBase> SFX_ResultVictory = nullptr;

	/** @brief 전투 패배 시 결과창 팝업 효과음 (암울한 톤) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|InGame")
	TObjectPtr<USoundBase> SFX_ResultDefeat = nullptr;
#pragma endregion 인게임 및 결과 (In-Game)
};
