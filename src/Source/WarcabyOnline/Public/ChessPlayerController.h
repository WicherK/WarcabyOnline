// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "CheckerPawn.h"
#include "ChessPlayerController.generated.h"

/**
 * 
 */
class UInputMappingContext;
class UInputAction;
class ACheckerPawn;
class ABoard;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShowBeforeMatchScreen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGameEnded, bool, isWin, bool, isOvertime, bool, isSurrender, int, expAfterMatch);

UCLASS()
class WARCABYONLINE_API AChessPlayerController : public APlayerController
{
	GENERATED_BODY()
private:
	UPROPERTY()
	ACheckerPawn* draggedPawn = nullptr;

	bool isDragging = false;

	bool isMovePending = false;

	FVector grabOffset;

public:
	AChessPlayerController();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EnhancedInput")
	UInputMappingContext* IMC_Context;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EnhancedInput")
	UInputAction* IA_Mouse;

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	ECheckerPawnType playerSide = ECheckerPawnType::None;

	TObjectPtr<ABoard> board;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Moving Pawn")
	float offset = 20.f;

	UPROPERTY(ReplicatedUsing = OnRep_EnemyNickname, VisibleAnywhere, BlueprintReadOnly)
	FString enemyNickname;

	UFUNCTION(BlueprintCallable)
	void OnRep_EnemyNickname();

	UPROPERTY(BlueprintAssignable)
	FShowBeforeMatchScreen ShowBeforeMatchScreen;

	UPROPERTY(BlueprintAssignable)
	FOnGameEnded OnEndGame;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_FinishedIntro();

	UFUNCTION(Client, Reliable)
	void Client_EndGame(bool isWin, bool isOvertime, bool isSurrender, int expAfterMatch);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_Surrender(ECheckerPawnType mySide);

	UFUNCTION(Client, Reliable)
	void Client_OnMoveFinished(bool isSuccess);

protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	void OnMouseClick(const FInputActionValue& Value);
	void OnMouseRelease(const FInputActionValue& Value);

	UFUNCTION(Server, Unreliable)
	void Server_UpdateDraggedPawnPosition(ACheckerPawn* checkerPawn, FVector newPos);

	UFUNCTION(Server, Reliable)
	void Server_TryMove(FIntPoint from, FIntPoint to);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
