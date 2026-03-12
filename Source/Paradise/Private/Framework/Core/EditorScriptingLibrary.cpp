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