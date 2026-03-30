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
		//궁극기 연출 종료 옵션이 꺼져있다면 여기서 더 이상 진행할 필요 없음
		if (!bStopUltimateEffect) return;

		//컨트롤러 및 카메라 매니저 캐싱 (실패 시 즉시 종료)
		AInGameController* InGamePC = Cast<AInGameController>(UGameplayStatics::GetPlayerController(OwnerActor, 0));
		if (!InGamePC) return;

		AParadiseCameraManager* CamMgr = Cast<AParadiseCameraManager>(InGamePC->PlayerCameraManager);
		if (!CamMgr) return;

		//[핵심 방어] 이 노티파이를 밟은 캐릭터가 '진짜 시전자'가 아니면 즉시 종료
		if (CamMgr->CurrentUltimateTarget != OwnerActor) return;

		// 6. 모든 관문을 통과했으므로 실제 연출 끄기 실행
		//UE_LOG(LogTemp, Warning, TEXT("[TestNotify] 궁극기 연출 종료 : %s"), *OwnerActor->GetName());
		CamMgr->StopUltimateCamera(OwnerActor);

		if (UUltimateEffectComponent* EffectComp = InGamePC->GetUltimateEffectComponent())
		{
			EffectComp->StopUltimateEffect();
		}
    }

	
}

void UTestNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
	{
		// 🌟 스테이트 틱이므로 bIsTick = true 전달
		Character->ExecuteAttackFromNotify(SocketName, SocketTarget, true);
	}
}
