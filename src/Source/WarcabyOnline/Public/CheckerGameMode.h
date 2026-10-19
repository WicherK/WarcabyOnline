#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ChessPlayerController.h"
#include "CheckerGameMode.generated.h"

UCLASS()
class WARCABYONLINE_API ACheckerGameMode : public AGameMode
{
	GENERATED_BODY()
private:
	UPROPERTY()
	TArray<AChessPlayerController*> connectedPlayers;

	UFUNCTION(BlueprintCallable)
	void HandleGameEnd(ECheckerPawnType winSide, bool isOvertime, bool isSurrender);

	void AfterMatchRequest(FString id, int levelPoints, int rankPoints, bool isWin);

	int32 playersWhoFinishedIntro = 0;

public:
	void FinishedIntro();

	void Surrender(ECheckerPawnType whichSide);

protected:
	virtual void BeginPlay() override;

	virtual FString InitNewPlayer(
		APlayerController* NewPlayerController,
		const FUniqueNetIdRepl& UniqueId,
		const FString& Options,
		const FString& Portal
	) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
