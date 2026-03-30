// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/Stage/ParadiseEnemyIconWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

#pragma region 생명주기 구현
void UParadiseEnemyIconWidget::NativeDestruct()
{
	//[최적화 및 안전성] 댕글링 포인터 방지를 위한 명시적 널 초기화
	Img_EnemyIcon = nullptr;
	Super::NativeDestruct();
}
#pragma endregion 생명주기 구현

#pragma region 로직 구현
void UParadiseEnemyIconWidget::SetupIcon(TSoftObjectPtr<UTexture2D> InFaceIcon)
{
	// [안전성] 컴포넌트 유효성 검사 필수
	if (!Img_EnemyIcon) return;

	if (InFaceIcon.IsNull())
	{
		// 데이터가 없을 경우 이미지를 비우거나 기본(Default) 이미지를 세팅하도록 방어 코드 작성
		Img_EnemyIcon->SetBrushFromTexture(nullptr);
		//UE_LOG(LogTemp, Warning, TEXT("⚠️ [EnemyIcon] 전달받은 텍스처(FaceIcon)가 비어있습니다."));
		return;
	}

	// [데이터 드리븐] Soft Pointer 동기 로드 (UI 렌더링 시점이므로 동기 로드 수행)
	UTexture2D* LoadedTexture = InFaceIcon.LoadSynchronous();
	if (LoadedTexture)
	{
		Img_EnemyIcon->SetBrushFromTexture(LoadedTexture);
	}
}
#pragma endregion 로직 구현