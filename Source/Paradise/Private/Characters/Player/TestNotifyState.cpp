// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/TestNotifyState.h"
#include "Characters/Base/CharacterBase.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "Framework/InGame/InGameController.h"
#include "Framework/Core/ParadiseCameraManager.h"
#include "Components/UltimateEffectComponent.h"
#include "Kismet/GameplayStatics.h"

void UTestNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (MeshComp)
    {
        if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
        {
            Character->ResetHitActors();
        }

		AActor* OwnerActor = MeshComp->GetOwner();
		//0327 김성현 - 궁극기 연출 해제 타이밍
		if (bStopUltimateEffect)
		{
			// 현재 액터를 소유한 로컬 플레이어 컨트롤러를 가져옴
			if (APlayerController* MainPC = UGameplayStatics::GetPlayerController(OwnerActor, 0))
			{
				if (AInGameController* InGamePC = Cast<AInGameController>(MainPC))
				{
					// 1. 카메라 매니저 줌인/슬로우모션 등 연출 끄기
					if (AParadiseCameraManager* CamMgr = Cast<AParadiseCameraManager>(InGamePC->PlayerCameraManager))
					{
						CamMgr->StopUltimateCamera();
					}

					// 2. 포스트 프로세스 볼륨 (배경 어두워짐 등) 연출 끄기
					if (UUltimateEffectComponent* EffectComp = InGamePC->GetUltimateEffectComponent())
					{
						EffectComp->StopUltimateEffect();
					}
				}
			}
		}
    }

	
}

void UTestNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
	{
		Character->CheckHit(SocketName, SocketTarget);
	}
}
