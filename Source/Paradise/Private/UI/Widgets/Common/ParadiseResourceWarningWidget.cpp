// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Common/ParadiseResourceWarningWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Animation/WidgetAnimation.h"
#include "Kismet/GameplayStatics.h"

#pragma region 생명주기
void UParadiseResourceWarningWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 닫기 버튼 이벤트 바인딩
	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.AddDynamic(this, &UParadiseResourceWarningWidget::HandleCloseButtonClicked);
	}

	// 초기에는 화면에서 숨겨둠 (필요할 때 ShowWarning으로 호출)
	SetVisibility(ESlateVisibility::Collapsed);
}

void UParadiseResourceWarningWidget::NativeDestruct()
{
	// 델리게이트 안전 해제
	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스
void UParadiseResourceWarningWidget::ShowWarning(const FText& ResourceName, UTexture2D* ResourceIcon, bool bIsExactMessage)
{
	// 1. 데이터 드라이븐 텍스트 포맷팅 (예: "골드" -> "골드이(가) 부족합니다!")
	//포맷팅 우회 여부 체크
	if (Text_WarningMessage)
	{
		if (bIsExactMessage)
		{
			// 🌟 true일 경우: "무기를 장착하지 않은 캐릭터가 있습니다." 문자열 그대로 출력
			Text_WarningMessage->SetText(ResourceName);
		}
		else
		{
			// false일 경우: 기존 데이터 드라이븐 텍스트 포맷팅 ("골드" -> "골드이(가) 부족합니다!")
			FText FormattedMessage = FText::Format(WarningMessageFormat, ResourceName);
			Text_WarningMessage->SetText(FormattedMessage);
		}
	}

	// 2. 아이템/재화 아이콘 렌더링
	if (Img_ResourceIcon)
	{
		if (ResourceIcon)
		{
			Img_ResourceIcon->SetBrushFromTexture(ResourceIcon);
			Img_ResourceIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			// 아이콘 데이터가 없으면 이미지를 숨김 처리 (안전장치)
			Img_ResourceIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 3. 위젯 가시성 활성화
	SetVisibility(ESlateVisibility::Visible);

	// 4. 경고 사운드 재생
	if (Sound_Warning)
	{
		UGameplayStatics::PlaySound2D(this, Sound_Warning);
	}

	// 5. 등장 애니메이션 재생 (UMG에서 만들어둔 팝업 애니메이션)
	if (Anim_PopupAppear)
	{
		PlayAnimation(Anim_PopupAppear);
	}
}
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
void UParadiseResourceWarningWidget::HandleCloseButtonClicked()
{
	// 팝업 숨김 처리
	SetVisibility(ESlateVisibility::Collapsed);

	// 컨트롤러(부모 위젯)에게 팝업이 닫혔음을 알림
	if (OnPopupClosed.IsBound())
	{
		OnPopupClosed.Broadcast();
	}
}
#pragma endregion 내부 로직
