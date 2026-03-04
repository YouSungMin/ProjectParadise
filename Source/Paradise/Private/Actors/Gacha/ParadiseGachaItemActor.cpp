// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Gacha/ParadiseGachaItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstance.h"

#pragma region 초기화 및 생명주기
AParadiseGachaItemActor::AParadiseGachaItemActor()
{
	// 비행 중일 때만 Tick을 활성화하여 성능을 최적화합니다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 1. 메시 컴포넌트 생성 및 터치/클릭 반응 설정
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;
	ItemMesh->SetGenerateOverlapEvents(false); // 불필요한 물리 연산 제거
	ItemMesh->SetCollisionProfileName(TEXT("UI")); // 터치 입력 감지를 위한 프로필

	// 모바일 터치 및 마우스 클릭 이벤트 바인딩
	ItemMesh->OnClicked.AddDynamic(this, &AParadiseGachaItemActor::HandleItemClicked);
	ItemMesh->OnInputTouchEnd.AddDynamic(this, &AParadiseGachaItemActor::HandleItemTouchEnd);

	// 2. 나이아가라 컴포넌트 생성
	RarityAuraEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RarityAuraEffect"));
	RarityAuraEffect->SetupAttachment(RootComponent);
	RarityAuraEffect->SetAutoActivate(false);
}

void AParadiseGachaItemActor::BeginPlay()
{
	Super::BeginPlay();
	// 상태 초기화
	CurrentState = EGachaItemState::Flying;
}

void AParadiseGachaItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 비행 상태일 때만 포물선 수학 연산 수행 (물리 엔진 미사용으로 압도적 최적화)
	if (CurrentState == EGachaItemState::Flying)
	{
		CurrentFlightTime += DeltaTime;
		float Alpha = FMath::Clamp(CurrentFlightTime / TotalFlightTime, 0.0f, 1.0f);

		// X, Y는 선형 보간 (Lerp)
		FVector CurrentPos = FMath::Lerp(StartLoc, EndLoc, Alpha);

		// Z(높이)는 사인(Sin) 곡선을 더해 포물선(Arc) 궤적 생성
		float HeightOffset = FMath::Sin(Alpha * PI) * MaxArcHeight;
		CurrentPos.Z += HeightOffset;

		SetActorLocation(CurrentPos);

		// 안착 완료 시 Tick 종료 및 상태 변경
		if (Alpha >= 1.0f)
		{
			CurrentState = EGachaItemState::Landed;
			SetActorTickEnabled(false);

			// 바닥에 안착하면 영롱한 아우라 재생 시작
			if (RarityAuraEffect)
			{
				RarityAuraEffect->Activate(true);
			}
		}
	}
}
#pragma endregion 초기화 및 생명주기

#pragma region 외부 인터페이스 (주입식 로직)
void AParadiseGachaItemActor::InitializeItemData(const FGachaResult& InResult, UMaterialInstance* InSilhouetteMat, UMaterialInstance* InRealMat)
{
	CachedItemData = InResult;
	CachedRealMaterial = InRealMat;

	// 시작 시점에는 무조건 실루엣으로 가림
	if (ItemMesh && InSilhouetteMat)
	{
		ItemMesh->SetMaterial(0, InSilhouetteMat);
	}
}

void AParadiseGachaItemActor::LaunchToTarget(FVector TargetLocation, float FlightDuration, float ArcHeight)
{
	StartLoc = GetActorLocation();
	EndLoc = TargetLocation;
	TotalFlightTime = FMath::Max(FlightDuration, 0.1f); // 0으로 나누기 방지
	CurrentFlightTime = 0.0f;
	MaxArcHeight = ArcHeight;

	// 비행 시작 (Tick 활성화)
	CurrentState = EGachaItemState::Flying;
	SetActorTickEnabled(true);
}

void AParadiseGachaItemActor::RevealItem()
{
	// 대기 상태(바닥에 꽂힘)일 때만 터치 리빌 허용
	if (CurrentState != EGachaItemState::Landed) return;

	CurrentState = EGachaItemState::Revealed;

	// 1. 진짜 머티리얼로 교체 (실루엣 해제)
	if (ItemMesh && CachedRealMaterial)
	{
		ItemMesh->SetMaterial(0, CachedRealMaterial);
	}

	// 2. 펑! 터지는 리빌 파티클 및 사운드 재생
	if (RevealVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), RevealVFX, GetActorLocation(), GetActorRotation());
	}
	if (RevealSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RevealSound, GetActorLocation());
	}

	// 3. 아우라 이펙트 끄기 (선택 사항)
	if (RarityAuraEffect)
	{
		RarityAuraEffect->Deactivate();
	}

	// 4. 상자 액터나 UI 매니저에게 "나 까졌어!" 하고 데이터 전달
	if (OnItemRevealed.IsBound())
	{
		OnItemRevealed.Broadcast(CachedItemData);
	}
}
#pragma endregion 외부 인터페이스 (주입식 로직)

#pragma region 내부 이벤트 핸들러 (Delegate Wrapper)
// 델리게이트로부터 이벤트를 받아서, 실제 연출 함수인 RevealItem을 호출합니다.
void AParadiseGachaItemActor::HandleItemClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	RevealItem();
}

void AParadiseGachaItemActor::HandleItemTouchEnd(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	RevealItem();
}
#pragma endregion 내부 이벤트 핸들러 (Delegate Wrapper)

