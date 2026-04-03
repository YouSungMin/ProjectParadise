// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Squad/ParadiseSquadSceneManager.h"
#include "Actors/Squad/ParadiseLobbyCharacterVisual.h"
#include "Framework/System/SquadSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/UnitStructs.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AParadiseSquadSceneManager::AParadiseSquadSceneManager()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	SceneCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SceneCamera"));
	SceneCamera->SetupAttachment(RootComponent);

	// 기획자가 에디터에서 보면서 드래그하기 쉽도록 Arrow(화살표) 컴포넌트 사용
	Point_Main = CreateDefaultSubobject<UArrowComponent>(TEXT("Point_Main"));
	Point_Main->SetupAttachment(RootComponent);
	Point_Main->ArrowColor = FColor::Yellow;

	Point_Sub1 = CreateDefaultSubobject<UArrowComponent>(TEXT("Point_Sub1"));
	Point_Sub1->SetupAttachment(RootComponent);

	Point_Sub2 = CreateDefaultSubobject<UArrowComponent>(TEXT("Point_Sub2"));
	Point_Sub2->SetupAttachment(RootComponent);
}

#pragma region 생명주기
void AParadiseSquadSceneManager::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시, 3개의 빈 마네킹을 지정된 화살표 위치에 미리 스폰하여 풀(Pool)에 담아둡니다.
	if (VisualActorClass)
	{
		UArrowComponent* Points[3] = { Point_Main, Point_Sub1, Point_Sub2 };

		for (int32 i = 0; i < 3; ++i)
		{
			FTransform SpawnTransform = Points[i]->GetComponentTransform();
			AParadiseLobbyCharacterVisual* NewVisual = GetWorld()->SpawnActor<AParadiseLobbyCharacterVisual>(VisualActorClass, SpawnTransform);

			if (NewVisual)
			{
				SpawnedVisuals.Add(i, NewVisual);
			}
		}
	}
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스
void AParadiseSquadSceneManager::RefreshSquadScene()
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	USquadSubsystem* SquadSys = GI ? GI->GetSubsystem<USquadSubsystem>() : nullptr;

	if (!SquadSys) return;

	const TArray<FName>& PlayerSquad = SquadSys->GetPlayerSquad();

	// 0, 1, 2 슬롯의 모델링을 업데이트합니다.
	UpdateSlotVisual(0, PlayerSquad.IsValidIndex(0) ? PlayerSquad[0] : NAME_None, Point_Main);
	UpdateSlotVisual(1, PlayerSquad.IsValidIndex(1) ? PlayerSquad[1] : NAME_None, Point_Sub1);
	UpdateSlotVisual(2, PlayerSquad.IsValidIndex(2) ? PlayerSquad[2] : NAME_None, Point_Sub2);

	//UE_LOG(LogTemp, Log, TEXT("[SquadScene] 3D 디오라마 모델링 갱신 완료"));
}
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
void AParadiseSquadSceneManager::UpdateSlotVisual(int32 SlotIndex, FName CharacterID, UArrowComponent* TargetPoint)
{
	// 미리 스폰해둔 마네킹을 찾습니다.
	AParadiseLobbyCharacterVisual** FoundVisual = SpawnedVisuals.Find(SlotIndex);
	if (!FoundVisual || !(*FoundVisual)) return;

	AParadiseLobbyCharacterVisual* VisualActor = *FoundVisual;

	// 비어있는 슬롯이면 옷을 벗기고(숨기고) 리턴
	if (CharacterID.IsNone())
	{
		VisualActor->SetVisual(nullptr, nullptr);
		return;
	}

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	// Data-Driven: GameInstance를 통해 에셋 데이터 테이블 조회
	if (FCharacterAssets* AssetData = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, CharacterID))
	{
		// 비동기 로드된 스켈레탈 메시와 애니메이션 장착
		USkeletalMesh* LoadedMesh = AssetData->SkeletalMesh.LoadSynchronous();
		UAnimSequence* LoadedAnim = AssetData->IdleAnim.LoadSynchronous(); // 데이터 테이블에 IdleAnim 필드가 있다고 가정

		VisualActor->SetVisual(LoadedMesh, LoadedAnim);
	}
	else
	{
		// 데이터가 없으면 일단 숨김
		VisualActor->SetVisual(nullptr, nullptr);
	}
}
#pragma endregion 내부 로직

