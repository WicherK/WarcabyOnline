#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "CheckerPawn.h"
#include "CheckerGameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnChanged, ECheckerPawnType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEndGame, ECheckerPawnType, winSide, bool, isOvertime, bool, isSurrender);

UCLASS()
class WARCABYONLINE_API ACheckerGameState : public AGameState
{
	GENERATED_BODY()
public:
	ACheckerGameState();

	UPROPERTY(Replicated, VisibleAnywhere)
	ECheckerPawnType whichTurn = ECheckerPawnType::White;

	UPROPERTY(Replicated, VisibleAnywhere)
	bool startTimers = false;

	UPROPERTY(Replicated, VisibleAnywhere)
	float whiteTime = 5 * 60; // 5 minutes

	UPROPERTY(Replicated, VisibleAnywhere)
	float blackTime = 5 * 60; // 5 minutes

	bool overtimeNoticed = false;

	FOnTurnChanged OnTurnChanged;

	FOnEndGame OnEndGame;

	void CheckEndGame();

	UPROPERTY(ReplicatedUsing = OnRep_Score, VisibleAnywhere)
	int blackPoints = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Score, VisibleAnywhere)
	int whitePoints = 0;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Score();
};
