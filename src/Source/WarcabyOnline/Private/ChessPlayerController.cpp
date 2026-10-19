#include "ChessPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Board.h"
#include "CheckerPawn.h"
#include "Net/UnrealNetwork.h"
#include "CheckerGameState.h"
#include "CheckerGameMode.h"

AChessPlayerController::AChessPlayerController()
{

}

void AChessPlayerController::OnRep_EnemyNickname()
{
	ShowBeforeMatchScreen.Broadcast();
}

void AChessPlayerController::Server_FinishedIntro_Implementation()
{
	ACheckerGameMode* GM = GetWorld()->GetAuthGameMode<ACheckerGameMode>();

	GM->FinishedIntro();
}

void AChessPlayerController::Client_EndGame_Implementation(bool isWin, bool isOvertime, bool isSurrender, int expAfterMatch)
{
	OnEndGame.Broadcast(isWin, isOvertime, isSurrender, expAfterMatch);
}

void AChessPlayerController::Server_Surrender_Implementation(ECheckerPawnType mySide)
{
	ACheckerGameMode* GM = GetWorld()->GetAuthGameMode<ACheckerGameMode>();

	GM->Surrender(playerSide);
}

void AChessPlayerController::Client_OnMoveFinished_Implementation(bool isSuccess)
{
	isMovePending = false;

	if (!isSuccess && draggedPawn)
	{
		// snap back
		FVector loc = board->GetSlot(draggedPawn->tilePosition)->worldPosition;
		loc.Z = draggedPawn->originalZ;

		draggedPawn->SetActorLocation(
			loc,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);

		draggedPawn->GetRootComponent()->ComponentVelocity = FVector::ZeroVector;
	}

	draggedPawn = nullptr;
}

void AChessPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	TArray<AActor*> FoundActors;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MainBoard"), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (ABoard* FoundBoard = Cast<ABoard>(Actor))
		{
			board = FoundBoard;
			break;
		}
	}
}

void AChessPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (isDragging && IsValid(draggedPawn) && !isMovePending)
	{
		FVector WorldLoc, WorldDir;

		if (DeprojectMousePositionToWorld(WorldLoc, WorldDir)) 
		{
			float planeZ = draggedPawn->originalZ + offset;
			float distance = (planeZ - WorldLoc.Z) / WorldDir.Z;

			if (FMath::IsNearlyZero(WorldDir.Z)) return;

			FVector target = WorldLoc + WorldDir * distance;
			target += grabOffset;
			FVector newLoc = FMath::VInterpTo(draggedPawn->GetActorLocation(), target, DeltaTime, 15.f);
			draggedPawn->SetActorLocation(newLoc);
			Server_UpdateDraggedPawnPosition(draggedPawn, newLoc);
		}
	}
}

void AChessPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input) return;

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(IMC_Context, 0);
		}
	}

	Input->BindAction(IA_Mouse, ETriggerEvent::Started, this, &AChessPlayerController::OnMouseClick);
	Input->BindAction(IA_Mouse, ETriggerEvent::Completed, this, &AChessPlayerController::OnMouseRelease);
}

void AChessPlayerController::OnMouseClick(const FInputActionValue& Value)
{
	ACheckerGameState* GS = GetWorld()->GetGameState<ACheckerGameState>();

	if (GS)
	{
		if (GS->whichTurn == playerSide && GS->startTimers)
		{
			FHitResult hit;

			GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, hit);

			if (!hit.bBlockingHit || !hit.GetActor())
			{
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("Hit: %s"), *hit.GetActor()->GetName());
			
			if (ACheckerPawn* hitPawn = Cast<ACheckerPawn>(hit.GetActor()))
			{
				if (hitPawn->pawnType != playerSide)
				{
					UE_LOG(LogTemp, Warning, TEXT("Not your pawn!"));
					return;
				}

				draggedPawn = hitPawn;
				isDragging = true;
				grabOffset = draggedPawn->GetActorLocation() - hit.Location;

				CurrentMouseCursor = EMouseCursor::GrabHandClosed;

				if (board)
				{
					board->ShowAvailableMoves(draggedPawn->tilePosition);
				}
			}
		}
		else
		{
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString("Cant move."));
		}
	}
}

void AChessPlayerController::OnMouseRelease(const FInputActionValue& Value)
{
	isDragging = false;
	CurrentMouseCursor = EMouseCursor::Default;

	grabOffset = FVector::ZeroVector;

	if (!draggedPawn) return;

	if (isMovePending) return;

	isMovePending = true;

	if (!board)
	{
		isMovePending = false;
		return;
	}

	// If pos is invalid dont event send request to server just snap it back locally
	if (!board->IsValidPos(draggedPawn->destinationTilePosition))
	{
		FVector loc = board->GetSlot(draggedPawn->tilePosition)->worldPosition;
		loc.Z = draggedPawn->originalZ;

		draggedPawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);

		draggedPawn = nullptr;
		isMovePending = false;
		return;
	}

	Server_TryMove(draggedPawn->tilePosition, draggedPawn->destinationTilePosition);

	board->HideAvailableMoves();
}

void AChessPlayerController::Server_UpdateDraggedPawnPosition_Implementation(ACheckerPawn* checkerPawn, FVector newPos)
{
	if (checkerPawn)
	{
		checkerPawn->SetActorLocation(newPos);
	}
}

void AChessPlayerController::Server_TryMove_Implementation(FIntPoint from, FIntPoint to)
{
	if (!board)
	{
		Client_OnMoveFinished(false);
		return;
	}

	bool success = board->TryMove(from, to);

	Client_OnMoveFinished(success);
}

void AChessPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChessPlayerController, playerSide);
	DOREPLIFETIME(AChessPlayerController, enemyNickname);
}
