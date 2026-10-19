#include "CheckerGameState.h"
#include "Net/UnrealNetwork.h"

ACheckerGameState::ACheckerGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACheckerGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || !startTimers) return;

	if (whichTurn == ECheckerPawnType::White)
	{
		if(whiteTime > 0) 
			whiteTime -= DeltaTime;
		else if(whiteTime <= 0 && !overtimeNoticed)
		{
			overtimeNoticed = true;
			whiteTime = 0;
			OnEndGame.Broadcast(ECheckerPawnType::Black, true, false);
		}
	}
	else
	{
		if(blackTime > 0)
			blackTime -= DeltaTime;
		else if(blackTime <= 0 && !overtimeNoticed)
		{
			overtimeNoticed = true;
			blackTime = 0;
			OnEndGame.Broadcast(ECheckerPawnType::White, true, false);
		}
	}
}

void ACheckerGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACheckerGameState, whitePoints);
	DOREPLIFETIME(ACheckerGameState, blackPoints);
	DOREPLIFETIME(ACheckerGameState, whichTurn);
	DOREPLIFETIME(ACheckerGameState, whiteTime);
	DOREPLIFETIME(ACheckerGameState, blackTime);
	DOREPLIFETIME(ACheckerGameState, startTimers);
}

void ACheckerGameState::CheckEndGame()
{
	if (!HasAuthority()) return;

	if (whitePoints >= 12)
	{
		OnEndGame.Broadcast(ECheckerPawnType::White, false, false);
	}

	if (blackPoints >= 12)
	{
		OnEndGame.Broadcast(ECheckerPawnType::Black, false, false);
	}
}

void ACheckerGameState::OnRep_Score()
{
	if (whitePoints >= 12)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString("White wins!"));
	
		OnEndGame.Broadcast(ECheckerPawnType::White, false, false);
	}

	if (blackPoints >= 12)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString("Black wins!"));
	
		OnEndGame.Broadcast(ECheckerPawnType::Black, false, false);
	}
}
