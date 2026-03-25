// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Core/EditorScriptingLibrary.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMeshSocket.h"


void UEditorScriptingLibrary::AddSocketToSkeleton(USkeleton* TargetSkeleton, FName SocketName, FName BoneName, FVector RelativeLocation, FRotator RelativeRotation, FVector RelativeScale)
{
	if (!TargetSkeleton) return;

	// 기존 소켓이 있으면 삭제 (덮어쓰기)
	for (int32 i = TargetSkeleton->Sockets.Num() - 1; i >= 0; --i)
	{
		if (TargetSkeleton->Sockets[i] && TargetSkeleton->Sockets[i]->SocketName == SocketName)
		{
			TargetSkeleton->Sockets.RemoveAt(i);
		}
	}

	// 새 소켓 생성 및 세팅
	USkeletalMeshSocket* NewSocket = NewObject<USkeletalMeshSocket>(TargetSkeleton);
	NewSocket->SocketName = SocketName;
	NewSocket->BoneName = BoneName;

	// 🚨 위치, 회전, 스케일 적용
	NewSocket->RelativeLocation = RelativeLocation;
	NewSocket->RelativeRotation = RelativeRotation;
	NewSocket->RelativeScale = RelativeScale;

	TargetSkeleton->Sockets.Add(NewSocket);
	TargetSkeleton->Modify();
}

void UEditorScriptingLibrary::BatchApplySoundConcurrency(const TArray<USoundBase*>& TargetSounds, USoundConcurrency* ConcurrencySetting)
{
	if (!ConcurrencySetting)
	{
		UE_LOG(LogTemp, Error, TEXT("적용할 동시성(Concurrency) 에셋이 비어있습니다!"));
		return;
	}

	int32 SuccessCount = 0;

	for (USoundBase* Sound : TargetSounds)
	{
		if (Sound)
		{
			// 1. 에셋 수정 플래그 켜기 (저장할 때 별표(*)가 뜨게 함)
			Sound->Modify();

			// 2. 기존에 설정된 동시성 세팅이 있다면 깔끔하게 지워줌
			Sound->ConcurrencySet.Empty();

			// 3. 우리가 지정한 동시성 세팅 추가
			Sound->ConcurrencySet.Add(ConcurrencySetting);

			SuccessCount++;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("✅ 총 %d개의 사운드 에셋에 동시성 적용이 완료되었습니다! (저장 버튼을 눌러 확정해주세요)"), SuccessCount);
}
