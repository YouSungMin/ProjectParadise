// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Squad/ParadiseLobbyInteractiveAvatar.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"
#include "Framework/System/SquadSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/UnitStructs.h"

#pragma region 생성자
AParadiseLobbyInteractiveAvatar::AParadiseLobbyInteractiveAvatar()
{
	// Tick 활성화 (스플라인 이동 처리를 위함)
	PrimaryActorTick.bCanEverTick = true;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	// 스플라인 세팅 (바닥 기준 경로)
	SplinePath = CreateDefaultSubobject<USplineComponent>(TEXT("SplinePath"));
	SplinePath->SetupAttachment(RootComponent);

	// 캐릭터 메시 세팅
	CharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	CharacterMesh->SetupAttachment(RootComponent);

	// 메시 자체가 터치 판정을 받도록 콜리전 최적화 세팅
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CharacterMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CharacterMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	CharacterMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // 마우스 클릭용 채널
}
#pragma endregion 생성자

#pragma region 생명주기
void AParadiseLobbyInteractiveAvatar::BeginPlay()
{
	Super::BeginPlay();

	// 클릭(터치) 델리게이트 바인딩
	if (CharacterMesh)
	{
		// PC/에디터 환경 클릭 바인딩
		CharacterMesh->OnClicked.AddDynamic(this, &AParadiseLobbyInteractiveAvatar::OnCharacterClicked);

		// 모바일 환경 터치 바인딩
		CharacterMesh->OnInputTouchBegin.AddDynamic(this, &AParadiseLobbyInteractiveAvatar::HandleTouchBegin);
	}
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (USquadSubsystem* SquadSys = GI->GetSubsystem<USquadSubsystem>())
		{
			SquadSys->OnPlayerSlotChanged.AddDynamic(
				this, &AParadiseLobbyInteractiveAvatar::OnPlayerSlotChanged);
		}
	}

	// 레벨 로드 시 초기 메인 캐릭터 갱신
	RefreshMainCharacter();
}

void AParadiseLobbyInteractiveAvatar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (USquadSubsystem* SquadSys = GI->GetSubsystem<USquadSubsystem>())
		{
			SquadSys->OnPlayerSlotChanged.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AParadiseLobbyInteractiveAvatar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!SplinePath || !CharacterMesh) return;

	// ── 앞으로 달리기 ────────────────────────────────────────────────
	if (bIsRunning)
	{
		const float TotalLength = SplinePath->GetSplineLength();
		const float RemainingDist = TotalLength - CurrentDistanceAlongSpline;

		const bool bInDecelerationZone = (RemainingDist <= DecelerationDistance);

		// ✅ 0까지 자연스럽게 내려감 (최솟값 제거)
		const float SpeedMultiplier = bInDecelerationZone
			? FMath::Clamp(RemainingDist / DecelerationDistance, 0.0f, 1.0f)
			: 1.0f;

		CurrentDistanceAlongSpline += RunSpeed * SpeedMultiplier * DeltaTime;

		// ✅ 속도가 거의 0이 되면 그 자리에서 Idle 전환 (스플라인 끝까지 안 가도 됨)
		if (SpeedMultiplier < 0.05f || CurrentDistanceAlongSpline >= TotalLength)
		{
			ResetToIdleState();
		}
		else
		{
			const FVector NewWorldLocation = SplinePath->GetLocationAtDistanceAlongSpline(
				CurrentDistanceAlongSpline, ESplineCoordinateSpace::World);

			const float TargetYaw = bInDecelerationZone
				? IdleFacingYaw
				: SplinePath->GetDirectionAtDistanceAlongSpline(
					CurrentDistanceAlongSpline, ESplineCoordinateSpace::World).Rotation().Yaw + MovementYawOffset;

			// ✅ 감속 비율에 따라 회전 보간 속도도 함께 줄어듦 (더 자연스러운 코너링)
			const float RotInterpSpeed = bInDecelerationZone
				? FMath::Lerp(IdleRotationInterpSpeed, 15.0f, SpeedMultiplier)
				: 15.0f;

			const FRotator TargetRotation = FRotator(0.0f, TargetYaw, 0.0f);
			const FRotator SmoothRotation = FMath::RInterpTo(
				CharacterMesh->GetComponentRotation(), TargetRotation, DeltaTime, RotInterpSpeed);

			CharacterMesh->SetWorldLocationAndRotation(NewWorldLocation, SmoothRotation);
		}
	}
	else if (bIsRotatingToIdle)
	{
		const FRotator TargetIdleRot = FRotator(0.0f, IdleFacingYaw, 0.0f);
		const FRotator CurrentRot = CharacterMesh->GetComponentRotation();
		const FRotator SmoothedRot = FMath::RInterpTo(
			CurrentRot, TargetIdleRot, DeltaTime, IdleRotationInterpSpeed);

		CharacterMesh->SetWorldRotation(SmoothedRot);

		if (FMath::Abs(FRotator::NormalizeAxis(SmoothedRot.Yaw - TargetIdleRot.Yaw)) < 1.0f)
		{
			CharacterMesh->SetWorldRotation(TargetIdleRot);
			bIsRotatingToIdle = false;
		}
	}
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스
void AParadiseLobbyInteractiveAvatar::RefreshMainCharacter()
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	USquadSubsystem* SquadSys = GI ? GI->GetSubsystem<USquadSubsystem>() : nullptr;

	if (!GI || !SquadSys) return;

	// Model(Subsystem)로부터 데이터 획득
	const TArray<FName>& PlayerSquad = SquadSys->GetPlayerSquad();
	const FName MainCharID = PlayerSquad.IsValidIndex(0) ? PlayerSquad[0] : NAME_None;

	if (MainCharID.IsNone())
	{
		CharacterMesh->SetSkeletalMesh(nullptr);
		CharacterMesh->SetAnimInstanceClass(nullptr);
		return;
	}

	if (FCharacterAssets* AssetData = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, MainCharID))
	{
		// 1. 에셋 로드 및 캐싱
		USkeletalMesh* LoadedMesh = AssetData->SkeletalMesh.LoadSynchronous();
		CachedIdleAnim = AssetData->IdleAnim.LoadSynchronous();
		CachedRunAnim = AssetData->RunAnim.LoadSynchronous(); // 구조체에 새로 추가하신 달리기 변수

		// 2. 메시 세팅 (데이터 드라이븐)
		CharacterMesh->SetSkeletalMesh(LoadedMesh);

		CharacterMesh->InitializeAnimScriptInstance(true);

		// 3. 애니메이션 블루프린트를 끄고, 단일 시퀀스 직접 재생 모드로 강제!
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

		// 4. 초기 상태로 초기화 (Idle 애니메이션 재생 포함)
		ResetToIdleState();
	}
}
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
void AParadiseLobbyInteractiveAvatar::OnCharacterClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	StartRunning();
}

void AParadiseLobbyInteractiveAvatar::HandleTouchBegin(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComp)
{
	StartRunning();
}

void AParadiseLobbyInteractiveAvatar::StartRunning()
{
	if (bIsRunning) return;

	bIsRunning = true;
	bIsRotatingToIdle = false;
	CurrentDistanceAlongSpline = 0.0f;

	if (CachedRunAnim && CharacterMesh)
	{
		CharacterMesh->PlayAnimation(CachedRunAnim, true);
	}
}

void AParadiseLobbyInteractiveAvatar::UpdateMeshAlongSpline(float Distance, float DeltaTime)
{
	const FVector NewWorldLocation = SplinePath->GetLocationAtDistanceAlongSpline(
		Distance, ESplineCoordinateSpace::World);

	const FVector SplineDir = SplinePath->GetDirectionAtDistanceAlongSpline(
		Distance, ESplineCoordinateSpace::World);

	FRotator TargetRotation = SplineDir.Rotation();
	TargetRotation.Yaw += MovementYawOffset;

	const FRotator SmoothRotation = FMath::RInterpTo(
		CharacterMesh->GetComponentRotation(), TargetRotation, DeltaTime, 15.0f);

	CharacterMesh->SetWorldLocationAndRotation(NewWorldLocation, SmoothRotation);
}

void AParadiseLobbyInteractiveAvatar::OnPlayerSlotChanged(int32 SlotIndex, FName NewPlayerID)
{
	// 메인 슬롯(0번)이 바뀔 때만 갱신
	if (SlotIndex == 0)
	{
		RefreshMainCharacter();
	}
}

void AParadiseLobbyInteractiveAvatar::ResetToIdleState()
{
	bIsRunning = false;
	bIsRotatingToIdle = false;
	CurrentDistanceAlongSpline = 0.0f;

	if (!SplinePath || !CharacterMesh) return;

	// 시작 위치로 복귀
	const FVector StartWorldLocation = SplinePath->GetLocationAtDistanceAlongSpline(
		0.0f, ESplineCoordinateSpace::World);
	CharacterMesh->SetWorldLocation(StartWorldLocation);

	if (CachedIdleAnim)
	{
		CharacterMesh->PlayAnimation(CachedIdleAnim, true);
	}

	// 회전은 Tick에서 부드럽게 보간
	bIsRotatingToIdle = true;
}
#pragma endregion 내부 로직
