#include "MultiplayerSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Online/OnlineSessionNames.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include <Kismet/GameplayStatics.h>

void UMultiplayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    mySessionName = FName("WarcabyOnlinev1.1");

    subsystem = IOnlineSubsystem::Get();

    if (subsystem)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("You are using: ") + subsystem->GetSubsystemName().ToString() + FString(" subsystem."));
        }

        // Setup session interface
        sessionInterface = subsystem->GetSessionInterface();

        if (sessionInterface)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Session interface is ok."));
            }

            // DELEGACI
            sessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UMultiplayerSubsystem::OnFindSessionComplete);
            sessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UMultiplayerSubsystem::OnDestroySessionComplete);
            sessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UMultiplayerSubsystem::OnCreateSessionComplete);
            sessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UMultiplayerSubsystem::OnJoinSessionComplete);
        }
    }
}

void UMultiplayerSubsystem::Deinitialize()
{
    if (sessionInterface)
    {
        sessionInterface->DestroySession(mySessionName);
    }
}

void UMultiplayerSubsystem::SetPlayerInfo()
{
    IOnlineIdentityPtr identityInterface = subsystem->GetIdentityInterface();

    if (identityInterface)
    {
        FUniqueNetIdPtr userID = identityInterface->GetUniquePlayerId(0);

        playerInfoStruct.playerID = userID->ToString();

        if (userID)
        {
            playerInfoStruct.nickname = identityInterface->GetPlayerNickname(*userID);
        }
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Fetch info about user: ") + playerInfoStruct.nickname);
    }

    // W TKI
    // Create JSON body
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField("id", playerInfoStruct.playerID);

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // Create HTTP Request
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

    Request->SetURL("https://YOUR_BASE.supabase.co/functions/v1/get-user-info");
    Request->SetVerb("POST");

    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("Authorization", "Bearer YOUR_API_KEY");

    Request->SetContentAsString(RequestBody);

    // Send request
    Request->OnProcessRequestComplete().BindLambda(
        [this](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Request failed"));
                return;
            }

            FString ResponseString = Res->GetContentAsString();
            UE_LOG(LogTemp, Warning, TEXT("Response: %s"), *ResponseString);

            // Parse JSON
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
                return;
            }


            // Get fields from JSON body
            TSharedPtr<FJsonObject> UserObject = JsonObject->GetObjectField("user");

            int32 UserId = UserObject->GetIntegerField("id");
            int32 RankPoints = UserObject->GetIntegerField("rankpoints");

            int32 Wins = 0;
            if (UserObject->HasField("wins") && !UserObject->TryGetNumberField("wins", Wins))
            {
                Wins = 0;
            }

            int32 Loses = 0;
            if (UserObject->HasField("loses") && !UserObject->TryGetNumberField("loses", Loses))
            {
                Loses = 0;
            }

            int32 Level = 0;
            if (UserObject->HasField("level") && !UserObject->TryGetNumberField("level", Level))
            {
                Level = 0;
            }

            int32 LevelPoints = 0;
            if (UserObject->HasField("levelPoints") && !UserObject->TryGetNumberField("levelPoints", LevelPoints))
            {
                LevelPoints = 0;
            }

            int32 NextLevelPoints = 0;
            if (UserObject->HasField("nextLevelPoints") && !UserObject->TryGetNumberField("nextLevelPoints", NextLevelPoints))
            {
                NextLevelPoints = 0;
            }

            playerInfoStruct.wins = Wins;
            playerInfoStruct.loses = Loses;
            playerInfoStruct.rankpoints = RankPoints;
            playerInfoStruct.level = Level;
            playerInfoStruct.levelpoints = LevelPoints;
            playerInfoStruct.nextLevelPoints = NextLevelPoints;

            isInfoSet = true;

            onPlayerInfoDetails.Broadcast(playerInfoStruct);

            UE_LOG(LogTemp, Warning, TEXT("UserId: %d"), UserId);
            UE_LOG(LogTemp, Warning, TEXT("RankPoints: %d"), RankPoints);
            UE_LOG(LogTemp, Warning, TEXT("Wins: %d"), Wins);
            UE_LOG(LogTemp, Warning, TEXT("Loses: %d"), Loses);
        }
    );

    Request->ProcessRequest();
}

void UMultiplayerSubsystem::FindGame()
{
    cancelRequested = false;
    isSearching = true;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Starting search for game."));
    }

    if (sessionInterface)
    {
        if (sessionInterface->GetNamedSession(mySessionName))
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Session already exists, we need to destroy it."));
            }

            findGameAfterDestroy = true;
            sessionInterface->DestroySession(mySessionName);
            return;
        }

        sessionSearch = MakeShared<FOnlineSessionSearch>();
        sessionSearch->bIsLanQuery = subsystem->GetSubsystemName() == "NULL";
        sessionSearch->MaxSearchResults = 9999;
        sessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
        sessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
        sessionSearch->QuerySettings.Set(FName("GAME_NAME"), FString("WARCABYONLINEV11"), EOnlineComparisonOp::Equals);

        sessionInterface->FindSessions(0, sessionSearch.ToSharedRef());
    }
}

void UMultiplayerSubsystem::Cancel()
{
    cancelRequested = true;
    isSearching = false;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Cancel requested."));
    }

    if (sessionInterface)
    {
        if (sessionSearch.IsValid())
        {
            sessionInterface->CancelFindSessions();

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Finding cancelled."));
            }
        }

        if (sessionInterface->GetNamedSession(mySessionName))
        {
            sessionInterface->DestroySession(mySessionName);

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Created session destroyed."));
            }
        }
    }
}

void UMultiplayerSubsystem::Quit()
{
    if (sessionInterface && sessionInterface->GetNamedSession(mySessionName))
    {
        goToMenuAfterDestroy = true;
        sessionInterface->DestroySession(mySessionName);
    }
}

void UMultiplayerSubsystem::GoToMenu()
{
    APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();

    if (!PC) return;

    if (PC->HasAuthority())
    {
        goToMenuAfterDestroy = false;
        isSearching = false;
        GetWorld()->ServerTravel("/Game/Levels/MenuMap?listen");
    }
    else
    {
        goToMenuAfterDestroy = false;
        PC->ClientTravel("/Game/Levels/MenuMap", TRAVEL_Absolute);
    }
}

void UMultiplayerSubsystem::CreateGame()
{
    if (sessionInterface->GetNamedSession(mySessionName))
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Session already exists, we need to destroy it."));
        }

        createGameAfterDestroy = true;
        sessionInterface->DestroySession(mySessionName);
        return;
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Creating own game."));
    }

    FOnlineSessionSettings sessionSettings;
    sessionSettings.bAllowJoinInProgress = true;
    sessionSettings.bAllowJoinViaPresence = true;
    sessionSettings.bIsLANMatch = subsystem->GetSubsystemName() == "NULL";
    sessionSettings.bShouldAdvertise = true;
    sessionSettings.bUsesPresence = true;
    sessionSettings.bUseLobbiesIfAvailable = true;
    sessionSettings.bIsDedicated = false;
    sessionSettings.NumPublicConnections = 2;
    sessionSettings.Set(FName("GAME_NAME"), FString("WARCABYONLINEV11"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    sessionInterface->CreateSession(0, mySessionName, sessionSettings);
}

void UMultiplayerSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
    if (cancelRequested)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Search cancelled."));
        }
        return;
    }

    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Finding sessions completed successfully."));
        }

        if (!sessionSearch)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Session search null."));
            }
            return;
        }

        if (sessionSearch->SearchResults.Num() > 0)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Found some active sessions."));
            }

            FString gameName;

            for (FOnlineSessionSearchResult result : sessionSearch->SearchResults)
            {
                if (!result.IsValid()) continue;

                if (result.Session.SessionSettings.Get(FName("GAME_NAME"), gameName))
                {
                    if (GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Found game. Key:") + gameName);
                    }

                    if (gameName.Equals("WARCABYONLINEV11"))
                    {
                        if (GEngine)
                        {
                            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Game matched criterias. We are joining."));
                        }

                        bool joinSuccess = sessionInterface->JoinSession(0, mySessionName, result);

                        if (!joinSuccess && GEngine)
                        {
                            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Error joining the session."));
                        }

                        return;
                    }
                }
            }

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("No session matched criterias."));
            }

            CreateGame();
        }
        else
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Search result is 0."));
            }

            CreateGame();
        }
    }
}

void UMultiplayerSubsystem::OnDestroySessionComplete(FName sessionName, bool bWasSuccessful)
{
    if (cancelRequested)
    {
        createGameAfterDestroy = false;
        findGameAfterDestroy = false;
        return;
    }

    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Session destroyed successfully."));
        }

        if (createGameAfterDestroy)
        {
            CreateGame();
        }
        else if (findGameAfterDestroy)
        {
            FindGame();
        }
        else if (goToMenuAfterDestroy)
        {
            GoToMenu();
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Couldn't destroy session."));
        }
    }
}

void UMultiplayerSubsystem::OnCreateSessionComplete(FName sessionName, bool bWasSuccessful)
{
    if (cancelRequested) return;

    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Session created successfully."));
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("We do the travel."));
        }
        
        GetWorld()->ServerTravel("/Game/Levels/MenuMap?listen");
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Couldn't create session."));
        }
    }
}

void UMultiplayerSubsystem::OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result)
{
    if (cancelRequested) return;

    if (result == EOnJoinSessionCompleteResult::Success)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Joined session successfully."));
        }

        FString address;
        bool success = sessionInterface->GetResolvedConnectString(sessionName, address);

        if (success)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Getting adress success."));
            }

            APlayerController* playerController = GetGameInstance()->GetFirstLocalPlayerController();

            if (playerController)
            {
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("We are connecting to: ") + address);
                    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Client do the travel."));
                }

                isSearching = false;
                playerController->ClientTravel(address, TRAVEL_Absolute);
            }
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Couldn't join session."));
        }
    }
}