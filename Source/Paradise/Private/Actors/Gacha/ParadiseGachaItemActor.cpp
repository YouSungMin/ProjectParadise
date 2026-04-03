// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Gacha/ParadiseGachaItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimSequence.h"
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

	// ── 캐릭터 리빌 메시 (기본 숨김) ─────────────────────────────────────
	RevealCharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RevealCharacterMesh"));
	RevealCharacterMesh->SetupAttachment(RootComponent);
	RevealCharacterMesh->SetVisibility(false);
	RevealCharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ── 장비 리빌 메시 (기본 숨김) ──────────────────────────────────────
	RevealEquipmentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RevealEquipmentMesh"));
	RevealEquipmentMesh->SetupAttachment(RootComponent);
	RevealEquipmentMesh->SetVisibility(false);
	RevealEquipmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	CacheGachaCamera();
}

void AParadiseGachaItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 비행 상태일 때만 포물선 수학 연산 수행 (물리 엔진 미사용으로 압도적 최적화)
	if (CurrentState != EGachaItemState::Flying) return;

	// ★ FlightSpeedMultiplier 적용 — 꾹 누름 시 2배속으로 날아감
	CurrentFlightTime += DeltaTime * FlightSpeedMultiplier;

	const float Alpha = FMath::Clamp(CurrentFlightTime / TotalFlightTime, 0.0f, 1.0f);

	// X, Y 선형 보간
	FVector CurrentPos = FMath::Lerp(StartLoc, EndLoc, Alpha);

	// Z 사인 곡선 포물선
	CurrentPos.Z += FMath::Sin(Alpha * PI) * MaxArcHeight;

	SetActorLocation(CurrentPos);

	// 착지 완료
	if (Alpha >= 1.0f)
	{
		CurrentState = EGachaItemState::Landed;
		SetActorTickEnabled(false);

		if (RarityAuraEffect)
		{
			RarityAuraEffect->Activate(true);
		}
		if (OnItemLanded.IsBound())
		{
			OnItemLanded.Broadcast();
		}
	}
}
#pragma endregion 초기화 및 생명주기

#pragma region 외부 인터페이스
void AParadiseGachaItemActor::InitializeItemData(const FGachaResult& InResult, UMaterialInstance* InSilhouetteMat, UMaterialInstance* InRealMat)
{
	CachedItemData = InResult;
	CachedRealMaterial = InRealMat;

	// 시작 시점에는 무조건 실루엣으로 가림
	if (ItemMesh && InSilhouetteMat)
	{
		ItemMesh->SetMaterial(0, InSilhouetteMat);
	}

	//0316 - 김성현 메쉬 관련 fatal error 잡기용 로그 추가 추후 삭제 예정
	// 캐릭터 메시 미리 세팅 (숨김 상태) — 리빌 시 SetVisibility만 하면 됨
	if (RevealCharacterMesh && CachedItemData.CharacterSkeletalMesh)
	{
		bool bIsSafe = true;
		const FReferenceSkeleton& RefSkel = CachedItemData.CharacterSkeletalMesh->GetRefSkeleton();
		const TArray<FTransform>& RefBonePose = RefSkel.GetRefBonePose();

		// 지뢰(스케일 0) 탐지기 작동!
		for (int32 i = 0; i < RefBonePose.Num(); ++i)
		{
			if (RefBonePose[i].GetScale3D().ContainsNaN())
			{
				bIsSafe = false;
				/*UE_LOG(LogTemp, Error,
					TEXT("[방어막 작동] %s의 %d번 뼈에 NaN 스케일이 있어 렌더링을 차단했습니다!"),
					*CachedItemData.CharacterSkeletalMesh->GetName(), i);*/
				break;
			}
		}

		if (bIsSafe)
		{
			RevealCharacterMesh->SetSkeletalMeshAsset(CachedItemData.CharacterSkeletalMesh);
		}
	}
	// 장비 메시 미리 세팅 (숨김 상태)
	else
	{
		if (RevealEquipmentMesh && CachedItemData.EquipmentStaticMesh)
		{
			RevealEquipmentMesh->SetStaticMesh(CachedItemData.EquipmentStaticMesh);
		}
	}
}

void AParadiseGachaItemActor::LaunchToTarget(FVector TargetLocation, float FlightDuration, float ArcHeight)
{
	bTouchEnabled = false;
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

	// 1. 구슬 숨기기
	if (ItemMesh)
	{
		ItemMesh->SetVisibility(false);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 2. 카메라 방향으로 회전 (캐싱된 값 사용)
	RotateTowardGachaCamera();

	// 3. 캐릭터 or 장비 메시 표시
	if (CachedItemData.bIsCharacter)
	{
		if (RevealCharacterMesh)
		{
			RevealCharacterMesh->SetWorldScale3D(CachedItemData.RevealMeshScale);
			RevealCharacterMesh->SetVisibility(true);
			if (CachedItemData.CharacterIdleAnim)
			{
				RevealCharacterMesh->PlayAnimation(CachedItemData.CharacterIdleAnim, true);
			}
		}
	}
	else
	{
		// 장비 — 스태틱 메시 표시
		if (RevealEquipmentMesh)
		{
			RevealEquipmentMesh->SetWorldScale3D(CachedItemData.RevealMeshScale);
			RevealEquipmentMesh->SetVisibility(true);
		}
	}

	// 4. 리빌 파티클 + 사운드
	if (TObjectPtr<UNiagaraSystem>* FxPtr = RevealVFXByRarity.Find(CachedItemData.PulledRarity))
	{
		if (FxPtr && *FxPtr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), *FxPtr, GetActorLocation(), GetActorRotation());
		}
	}

	// 5. 아우라 끄기
	if (RarityAuraEffect)
	{
		RarityAuraEffect->Deactivate();
	}

	// 6. 완료 이벤트 방송
	if (OnItemRevealed.IsBound())
	{
		OnItemRevealed.Broadcast(CachedItemData);
	}
}

void AParadiseGachaItemActor::EnableTouch()
{
	bTouchEnabled = true;

	if (ItemMesh)
	{
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void AParadiseGachaItemActor::SetFlightSpeedMultiplier(float InMultiplier)
{
	FlightSpeedMultiplier = FMath::Max(InMultiplier, 0.1f);
}
#pragma endregion 외부 인터페이스 

#pragma region 내부 로직 구현
void AParadiseGachaItemActor::CacheGachaCamera()
{
	// 태그로 검색 — BeginPlay에서 1회만 호출되므로 성능 부담 없음
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), GachaCameraTag, FoundActors);

	if (FoundActors.Num() > 0)
	{
		CachedGachaCamera = FoundActors[0];
	}
	/*else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("⚠️ [GachaItemActor] 태그 '%s'를 가진 카메라를 찾지 못했습니다. 레벨 액터에 태그를 추가하세요."),
			*GachaCameraTag.ToString());
	}*/
}

void AParadiseGachaItemActor::RotateTowardGachaCamera()
{
	if (!CachedGachaCamera.IsValid()) return;

	const FVector CamLoc = CachedGachaCamera.Get()->GetActorLocation();
	const FVector ToCamera = (CamLoc - GetActorLocation()).GetSafeNormal();
	const FRotator LookRot = FRotationMatrix::MakeFromX(ToCamera).Rotator();

	// Yaw만 회전 — 캐릭터가 기울지 않도록
	SetActorRotation(FRotator(0.0f, LookRot.Yaw - 90.0f, 0.0f));
}
#pragma endregion 내부 로직 구현

#pragma region 내부 이벤트 핸들러
// 델리게이트로부터 이벤트를 받아서, 실제 연출 함수인 RevealItem을 호출합니다.
void AParadiseGachaItemActor::HandleItemClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	RevealItem();
}

void AParadiseGachaItemActor::HandleItemTouchEnd(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	// 모든 구슬 착지 전 터치 완전 차단
	if (!bTouchEnabled) return;
	RevealItem();
}
#pragma endregion 내부 이벤트 핸들러

