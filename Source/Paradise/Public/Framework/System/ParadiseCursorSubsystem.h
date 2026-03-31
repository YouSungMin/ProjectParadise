// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
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
     * @brief 커서 위치를 갱신합니다.
     * @param InPosition 화면 좌표
     */
    UFUNCTION(BlueprintCallable, Category = "Paradise|UI|Cursor")
    void UpdateCursorPosition(FVector2D InPosition);

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
#pragma endregion 내부 데이터
};
