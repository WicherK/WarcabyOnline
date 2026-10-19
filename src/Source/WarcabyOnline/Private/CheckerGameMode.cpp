#include "CheckerGameMode.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "EngineUtils.h"
#include "CheckerGameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

void ACheckerGameMode::Surrender(ECheckerPawnType whichSide)
{
	HandleGameEnd(whichSide == ECheckerPawnType::White ? ECheckerPawnType::Black : ECheckerPawnType::White, false, true);
}

void ACheckerGameMode::FinishedIntro()
{
	playersWhoFinishedIntro += 1;

	if (playersWhoFinishedIntro >= 2)
	{
		ACheckerGameState* GS = GetGameState<ACheckerGameState>();

		if (!GS) return;

		GS->startTimers = true;
	}
}

void ACheckerGameMode::BeginPlay()
{
	Super::BeginPlay();

	ACheckerGameState* GS = GetGameState<ACheckerGameState>();
	if (GS)
	{
		GS->OnEndGame.AddDynamic(this, &ACheckerGameMode::HandleGameEnd);
	}
}

FString ACheckerGameMode::InitNewPlayer(
	APlayerController* NewPlayerController,
	const FUniqueNetIdRepl& UniqueId,
	const FString& Options,
	const FString& Portal)
{
	AChessPlayerController* PC = Cast<AChessPlayerController>(NewPlayerController);

	if (PC)
	{
		if (connectedPlayers.Num() == 0)
		{
			ECheckerPawnType randomSide = FMath::RandBool()
				? ECheckerPawnType::White
				: ECheckerPawnType::Black;

			PC->playerSide = randomSide;
		}
		else
		{
			ECheckerPawnType firstSide = connectedPlayers[0]->playerSide;

			PC->playerSide =
				(firstSide == ECheckerPawnType::White)
				? ECheckerPawnType::Black
				: ECheckerPawnType::White;
		}
		
		connectedPlayers.Add(PC);
	}

	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

void ACheckerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (connectedPlayers.Num() == 2)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Setting enemy nicknames."));

		connectedPlayers[1]->enemyNickname = connectedPlayers[0]->GetPlayerState<APlayerState>()->GetPlayerName();
		connectedPlayers[0]->enemyNickname = connectedPlayers[1]->GetPlayerState<APlayerState>()->GetPlayerName();
	
		connectedPlayers[0]->OnRep_EnemyNickname();
		connectedPlayers[1]->OnRep_EnemyNickname();
	}
}

AActor* ACheckerGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	AChessPlayerController* PC = Cast<AChessPlayerController>(Player);
	if (!PC)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	FName desiredTag =
		(PC->playerSide == ECheckerPawnType::White)
		? FName("White")
		: FName("Black");

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* start = *It;

		if (start && start->ActorHasTag(desiredTag))
		{
			return start;
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void ACheckerGameMode::HandleGameEnd(ECheckerPawnType winSide, bool isOvertime, bool isSurrender)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString("Handling end game!"));

	for (AChessPlayerController* PC : connectedPlayers)
	{
		if (!IsValid(PC)) continue;

		APlayerState* PS = PC->GetPlayerState<APlayerState>();
		if (!PS || !PS->GetUniqueId().IsValid()) continue;

		FString id = PS->GetUniqueId()->ToString();

		bool isWin = (PC->playerSide == winSide);

		int expAfterMatch = isWin ? 350 : 150;

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, id + (isWin ? FString(" lost!") : FString(" won!")));

		PC->Client_EndGame(isWin, isOvertime, isSurrender, expAfterMatch);

		AfterMatchRequest(id, expAfterMatch, isWin ? 25 : -25, isWin);
	}
}

void ACheckerGameMode::AfterMatchRequest(FString id, int levelPoints, int rankPoints, bool isWin)
{
	// Create JSON body
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField("id", id);
	JsonObject->SetNumberField("levelPoints", levelPoints);
	JsonObject->SetNumberField("rankPoints", rankPoints);
	JsonObject->SetBoolField("isWin", isWin);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// Create HTTP Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	Request->SetURL("https://YOUR_BASE.supabase.co/functions/v1/match-end");
	Request->SetVerb("POST");

	Request->SetHeader("Content-Type", "application/json");
	Request->SetHeader("Authorization", "Bearer YOUR_API_KEY");

	Request->SetContentAsString(RequestBody);

	// Bind response function
	Request->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
		{
			if (!bSuccess || !Res.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("Request failed"));
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Request failed"));
				return;
			}

			FString ResponseString = Res->GetContentAsString();
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, ResponseString);
			UE_LOG(LogTemp, Warning, TEXT("Response: %s"), *ResponseString);
		}
	);

	Request->ProcessRequest();
}
