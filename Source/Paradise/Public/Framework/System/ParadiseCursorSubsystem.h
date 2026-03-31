// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "ParadiseCursorSubsystem.generated.h"

#pragma region 전방 선언
class UParadiseCursorWidget;
class UTexture2D;
#pragma endregion 전방 선언
/**
 * @class UParadiseCursorSubsystem
 * @brief 프로젝트 전반의 소프트웨어 커서를 전역으로 관리하는 서브시스템입니다.
 * @details 타이틀, 로비, 인게임 어디서든 재사용 가능하며 모바일에서는 자동으로 비활성화됩니다.
 */
UCLASS()
class PARADISE_API UParadiseCursorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

#pragma region 외부 인터페이스
public:
    /**
     * @brief 커서 위젯을 생성하고 화면에 표시합니다.
     * @details 모바일 플랫폼에서는 자동으로 무시됩니다.
     * @param InWidgetClass 사용할 커서 위젯 클래스
     * @param InCursorTexture 커서 텍스처
     * @param InPlayerController 소유 플레이어 컨트롤러
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Cursor")
    void InitializeCursor(
        TSubclassOf<UParadiseCursorWidget> InWidgetClass,
        UTexture2D* InCursorTexture,
        APlayerController* InPlayerController
    );

    /**
     * @brief 커서 표시 여부를 설정합니다.
     * @param bShow true면 표시, false면 숨김
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Cursor")
    void ShowCursor(bool bShow);

    /**
     * @brief 메뉴(설정, 결과창 등)가 열렸을 때 키보드 입력이 들어와도 커서가 숨겨지지 않도록 강제합니다.
     * @param bForce true면 강제 표시, false면 일반 로직으로 복귀
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Cursor")
    void SetCursorForceVisible(bool bForce);

    /**
     * @brief 현재 플랫폼이 모바일인지 확인합니다.
     * @return 모바일이면 true
     */
    UFUNCTION(BlueprintPure, Category = "Paradise|UI|Cursor")
    bool IsMobilePlatform() const;

    /** @brief 커서 위젯을 제거하고 정리합니다. */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Cursor")
    void CleanupCursor();
#pragma endregion 외부 인터페이스

#pragma region 내부 데이터
private:
    /** @brief 소프트웨어 커서 위젯 인스턴스 */
    UPROPERTY()
    TObjectPtr<UParadiseCursorWidget> CursorWidgetInstance = nullptr;

    /** @brief 소유 플레이어 컨트롤러 캐싱 */
    TWeakObjectPtr<APlayerController> CachedPlayerController = nullptr;

    /** @brief 게임 일시정지와 무관하게 동작하는 코어 레벨 커서 갱신 핸들 */
    FTSTicker::FDelegateHandle CursorTickHandle;

    /** @brief 이전 프레임 마우스 위치 (이동 감지용) */
    FVector2D LastCursorPos = FVector2D::ZeroVector;

    /** @brief 커서가 현재 숨김 상태인지 여부 */
    bool bCursorHidden = false;
#pragma endregion 내부 데이터
private:
    /** @brief 매 프레임 마우스 위치를 추적하여 커서 위젯을 갱신합니다. */
    bool UpdateCursorTick(float DeltaTime);

    /**
     * @brief Slate 절대 좌표를 뷰포트 상대 좌표로 변환합니다.
     * @param OutPosition 변환된 뷰포트 상대 좌표
     * @return 위치 획득 성공 여부
     */
    bool GetCursorPositionInViewportSpace(FVector2D& OutPosition) const;
};
