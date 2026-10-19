#include "Board.h"
#include "BoardSlot.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "CheckerGameState.h"
#include "Kismet/GameplayStatics.h"
#include <ranges>

ABoard::ABoard()
{
	PrimaryActorTick.bCanEverTick = true;

	staticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardStaticMesh"));
	if (staticMesh)
		SetRootComponent(staticMesh);

	staticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	refPoint = CreateDefaultSubobject<USceneComponent>(TEXT("refPoint"));
	refPoint->SetUsingAbsoluteLocation(false);
	refPoint->SetupAttachment(staticMesh);
}

void ABoard::BeginPlay()
{
	Super::BeginPlay();

	boardState.Init(FBoardSlotData(), 64);

	if (HasAuthority())
	{
		SpawnSlots();
		SpawnPawns();
	}

	ForceNetUpdate();
}

void ABoard::SpawnSlots()
{
	// Get reference point position for spawning board slots and board pawns
	FVector refPosition = refPoint->GetComponentLocation();

	for (int32 y = 0; y < 8; y++)
	{
		for (int32 x = 0; x < 8; x++)
		{
			FVector worldPosition = refPosition + FVector(x * spaceOffset, y * spaceOffset, 0.f);

			FIntPoint tilePosition = FIntPoint(x, y);

			ABoardSlot* slotObject = GetWorld()->SpawnActor<ABoardSlot>(
				BP_BoardSlot,
				worldPosition,
				FRotator::ZeroRotator
			);

			FBoardSlotData boardSlotData;
			boardSlotData.worldPosition = worldPosition;
			boardSlotData.tilePosition = tilePosition;

			if (!slotObject)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to spawn BoardSlot!"));
				continue;
			}

			slotObject->tilePosition = tilePosition;

			if(!boardState.IsValidIndex(ToIndex(tilePosition)))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to add board slot to board state!"));
				continue;
			}

			boardState[ToIndex(tilePosition)] = boardSlotData;

			//DrawDebugSphere(
			//	GetWorld(),
			//	worldPosition,
			//	5.0f,        // radius
			//	12,           // segments
			//	FColor::Red,
			//	true,         // persistent
			//	-1.0f,        // lifetime (-1 = forever)
			//	0,
			//	1.0f          // thickness
			//);
		}
	}
}

void ABoard::SpawnPawns()
{
	for (auto& slot : boardState)
	{
		if (slot.tilePosition.Y == 3 || slot.tilePosition.Y == 4) continue;
		
		if ((slot.tilePosition.X + slot.tilePosition.Y) % 2 == 0)
		{
			ACheckerPawn* pawn = GetWorld()->SpawnActor<ACheckerPawn>(
				BP_CheckerPawn,
				slot.worldPosition,
				FRotator::ZeroRotator
			);

			FCheckerPawnData checkerPawnData;
			checkerPawnData.tilePosition = slot.tilePosition;

			if (!pawn) continue;

			pawn->tilePosition = slot.tilePosition;

			if (slot.tilePosition.Y < 3)
			{
				pawn->pawnType = ECheckerPawnType::White;
				checkerPawnData.pawnType = ECheckerPawnType::White;
			}
			else if (slot.tilePosition.Y > 4)
			{
				pawn->pawnType = ECheckerPawnType::Black;
				checkerPawnData.pawnType = ECheckerPawnType::Black;
			}

			pawn->SetColor(pawn->pawnType == ECheckerPawnType::White ? FLinearColor::White : FLinearColor::Black);

			slot.holdedPawn = checkerPawnData;
			slot.isHoldingPawn = true;
		}
	}
}

ACheckerPawn* ABoard::GetPawnActorAt(FIntPoint pos)
{
	for (TActorIterator<ACheckerPawn> It(GetWorld()); It; ++It)
	{
		if (It->tilePosition == pos)
		{
			return *It;
		}
	}
	return nullptr;
}

ABoardSlot* ABoard::GetBoardSlotActorAt(FIntPoint pos)
{
	for (TActorIterator<ABoardSlot> It(GetWorld()); It; ++It)
	{
		if (It->tilePosition == pos)
		{
			return *It;
		}
	}
	return nullptr;
}

FBoardSlotData* ABoard::GetSlot(FIntPoint pos)
{
	if (!IsValidPos(pos)) return nullptr;

	return &boardState[ToIndex(pos)];
}

TArray<FIntPoint> ABoard::GetNormalMoves(FIntPoint from)
{
	TArray<FIntPoint> moves;

	FCheckerPawnData pawnInfo = GetSlot(from)->holdedPawn;

	FIntPoint pos = from;

	TArray<FIntPoint> directions =
	{
		FIntPoint(-1, 1),
		FIntPoint(1, 1),
		FIntPoint(-1, -1),
		FIntPoint(1, -1)
	};

	// Normal pawn moves
	if (!pawnInfo.isKing)
	{
		int32 direction = (pawnInfo.pawnType == ECheckerPawnType::White) ? 1 : -1;

		TArray<FIntPoint> pawnDirs =
		{
			FIntPoint(-1, direction),
			FIntPoint(1, direction)
		};

		for (const FIntPoint& dir : pawnDirs)
		{
			FIntPoint target = pos + dir;
			FBoardSlotData* slot = GetSlot(target);

			if (!slot) continue;

			// NORMAL MOVE = empty tile
			if (!slot->isHoldingPawn)
			{
				moves.Add(target);
			}
		}
	}
	// King normal moves
	else
	{
		for (const FIntPoint& dir : directions)
		{
			FIntPoint current = pos + dir;

			for (int32 step : std::views::iota(1, 8)) // U¯YCIE RANGES
			{
				const FBoardSlotData* slot = GetSlot(current);
				if (!slot) break;

				if (slot->isHoldingPawn)
				{
					break;
				}

				moves.Add(current);

				current += dir;
			}
		}
	}

	return moves;
}

TArray<FIntPoint> ABoard::GetCaptureMoves(FIntPoint from)
{
	TArray<FIntPoint> moves;

	FCheckerPawnData pawnInfo = GetSlot(from)->holdedPawn;

	FIntPoint pos = pawnInfo.tilePosition;

	TArray<FIntPoint> directions =
	{
		FIntPoint(-1, 1),
		FIntPoint(1, 1),
		FIntPoint(-1, -1),
		FIntPoint(1, -1)
	};

	// Capture pawn moves
	if (!pawnInfo.isKing)
	{
		for (const FIntPoint& dir : directions)
		{
			FIntPoint enemyTile = pos + dir;
			FIntPoint landingTile = enemyTile + dir;

			const FBoardSlotData* enemySlot = GetSlot(enemyTile);
			const FBoardSlotData* landingSlot = GetSlot(landingTile);

			if (!enemySlot || !landingSlot) continue;

			if (enemySlot->isHoldingPawn &&
				enemySlot->holdedPawn.pawnType != pawnInfo.pawnType &&
				!landingSlot->isHoldingPawn)
			{
				moves.Add(landingTile);
			}
		}
	}
	// King capture moves
	else
	{
		for (const FIntPoint& dir : directions)
		{
			FIntPoint current = pos + dir;
			bool foundEnemy = false;

			while (true)
			{
				const FBoardSlotData* slot = GetSlot(current);
				if (!slot) break;

				if (slot->isHoldingPawn)
				{
					if (!foundEnemy &&
						slot->holdedPawn.pawnType != pawnInfo.pawnType)
					{
						foundEnemy = true;
					}
					else
					{
						break; // own pawn or second piece
					}
				}
				else
				{
					if (foundEnemy)
					{
						moves.Add(current); // valid landing
					}
				}

				current += dir;
			}
		}
	}

	return moves;
}

bool ABoard::CanCapture(ECheckerPawnType playerSide)
{
	for (const FBoardSlotData& slot : boardState)
	{
		if (!slot.isHoldingPawn) continue;

		if (slot.holdedPawn.pawnType != playerSide) continue;

		TArray<FIntPoint> captures = GetCaptureMoves(slot.tilePosition);

		if (captures.Num() > 0)
		{
			return true; // at least one pawn can capture
		}
	}

	return false;
}

void ABoard::Multicast_PlayMoveSound_Implementation(FVector location)
{
	if (moveSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, moveSound, location);
		UGameplayStatics::PlaySoundAtLocation(this, tapperSound, location);
	}
}

void ABoard::ShowAvailableMoves(FIntPoint from)
{
	TArray<FIntPoint> normalMoves = GetNormalMoves(from);
	TArray<FIntPoint> captureMoves = GetCaptureMoves(from);

	// Clear previous highlights
	HideAvailableMoves();

	// Highlight normal moves
	if(captureMoves.Num() <= 0)
	{
		for (const FIntPoint& move : normalMoves)
		{
			ABoardSlot* slot = GetBoardSlotActorAt(move);

			if (slot)
			{
				slot->SetColor(FLinearColor::White);
				slot->SetHighlight(true);
			}
		}
	}

	// Highlight capture moves
	for (const FIntPoint& move : captureMoves)
	{
		ABoardSlot* slot = GetBoardSlotActorAt(move);

		if (slot)
		{
			slot->SetColor(FLinearColor::Red);
			slot->SetHighlight(true);
		}
	}
}

void ABoard::OnRep_BoardState()
{
	UE_LOG(LogTemp, Warning, TEXT("BoardState replicated on client"));
}

void ABoard::HideAvailableMoves()
{
	for (TActorIterator<ABoardSlot> It(GetWorld()); It; ++It)
	{
		ABoardSlot* slot = *It;

		if (slot)
		{
			slot->SetHighlight(false);
		}
	}
}

void ABoard::TryPromotePawn(FBoardSlotData* slot, ACheckerPawn* pawn)
{
	if (!slot || !pawn) return;

	FCheckerPawnData& pawnInfo = slot->holdedPawn;

	if (pawnInfo.isKing) return;

	// Promotion rows
	bool reachedEnd = false;

	if (pawnInfo.pawnType == ECheckerPawnType::White && pawnInfo.tilePosition.Y == 7)
	{
		reachedEnd = true;
	}
	else if (pawnInfo.pawnType == ECheckerPawnType::Black && pawnInfo.tilePosition.Y == 0)
	{
		reachedEnd = true;
	}

	if (reachedEnd)
	{
		pawnInfo.isKing = true;
		pawn->isKing = true;
		pawn->Promote();

		UE_LOG(LogTemp, Warning, TEXT("Pawn promoted to KING!"));
	}
}

bool ABoard::TryMove(FIntPoint from, FIntPoint to)
{
	ACheckerPawn* pawn = GetPawnActorAt(from);

	if (!pawn)
	{
		UE_LOG(LogTemp, Error, TEXT("Pawn actor not found at (%d,%d)"), from.X, from.Y);
		return false;
	}

	bool changeTurn = true;

	// Only server should validate & execute
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("TryMove called on client!"));
		if (pawn)
		{
			FVector loc = GetSlot(from)->worldPosition;
			loc.Z = pawn->originalZ;

			pawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);
			pawn->tilePosition = from;
		}
		return false;
	}

	// Validate positions
	if (!IsValidPos(from) || !IsValidPos(to))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid positions"));

		if (pawn)
		{
			FVector loc = GetSlot(from)->worldPosition;
			loc.Z = pawn->originalZ;

			pawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);
			pawn->tilePosition = from;
		}

		return false;
	}

	int32 fromIdx = ToIndex(from);
	int32 toIdx = ToIndex(to);

	FBoardSlotData* fromSlot = GetSlot(from);
	FBoardSlotData* toSlot = GetSlot(to);

	if (from == to)
	{
		UE_LOG(LogTemp, Error, TEXT("Same position"));
		if (pawn)
		{
			FVector loc = GetSlot(from)->worldPosition;
			loc.Z = pawn->originalZ;

			pawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);
			pawn->tilePosition = from;
		}
		return false;
	}

	// Must have pawn
	if (!fromSlot->isHoldingPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("No pawn on source tile"));

		if (pawn)
		{
			FVector loc = GetSlot(from)->worldPosition;
			loc.Z = pawn->originalZ;

			pawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);
			pawn->tilePosition = from;
		}

		return false;
	}

	if (!toSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid slot"));

		if (pawn)
		{
			FVector loc = GetSlot(from)->worldPosition;
			loc.Z = pawn->originalZ;

			pawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);
			pawn->tilePosition = from;
		}

		return false;
	}

	// Destination must be empty
	if (toSlot->isHoldingPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Target occupied"));

		if (pawn)
		{
			FVector loc = GetSlot(from)->worldPosition;
			loc.Z = pawn->originalZ;

			pawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);
			pawn->tilePosition = from;
		}

		return false;
	}

	// Check if move is legal
	TArray<FIntPoint> normalMoves = GetNormalMoves(from);
	TArray<FIntPoint> captureMoves = GetCaptureMoves(from);

	if (!normalMoves.Contains(to) && !captureMoves.Contains(to))
	{
		UE_LOG(LogTemp, Warning, TEXT("Illegal move"));
		
		if (pawn)
		{
			FVector loc = GetSlot(from)->worldPosition;
			loc.Z = pawn->originalZ;

			pawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);
			pawn->tilePosition = from;
		}

		return false;
	}

	// Force capture by this pawn
	bool isCaptureMove = captureMoves.Contains(to);

	if (!isCaptureMove && captureMoves.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("You must capture"));

		if (pawn)
		{
			FVector loc = GetSlot(from)->worldPosition;
			loc.Z = pawn->originalZ;

			pawn->SetActorLocation(loc, false, nullptr, ETeleportType::TeleportPhysics);
			pawn->tilePosition = from;
		}

		return false;
	}

	// If it was capture update score and delete captured actor pawn
	if (captureMoves.Contains(to))
	{
		FIntPoint dir = FIntPoint(
			FMath::Sign(to.X - from.X),
			FMath::Sign(to.Y - from.Y)
		);

		FIntPoint current = from + dir;
		FIntPoint capturedPos = FIntPoint(-1, -1);

		while (current != to)
		{
			FBoardSlotData* slot = GetSlot(current);

			if (slot && slot->isHoldingPawn)
			{
				capturedPos = current;
				break; // first enemy
			}

			current += dir;
		}

		FBoardSlotData* capturedSlot = GetSlot(capturedPos);

		if (capturedSlot && capturedSlot->isHoldingPawn)
		{
			// Destroy actor
			ACheckerPawn* capturedPawn = GetPawnActorAt(capturedPos);

			if (capturedPawn)
			{
				capturedPawn->Destroy();
			}

			ACheckerGameState* GS = GetWorld()->GetGameState<ACheckerGameState>();
			
			if (GS)
			{
				if (GS->whichTurn == ECheckerPawnType::White)
					GS->whitePoints += 1;
				else if (GS->whichTurn == ECheckerPawnType::Black)
					GS->blackPoints += 1;

				GS->CheckEndGame();
			}

			// Clear board data
			capturedSlot->holdedPawn = FCheckerPawnData();
			capturedSlot->isHoldingPawn = false;

			UE_LOG(LogTemp, Warning, TEXT("Captured pawn at (%d,%d)"),
				capturedPos.X, capturedPos.Y);
		}
	}
	else
	{
		if (CanCapture(pawn->pawnType))
		{
			UE_LOG(LogTemp, Warning, TEXT("There is capture possible, you cant do normal move."));
			if (pawn)
			{
				FVector loc = GetSlot(from)->worldPosition;
				loc.Z = pawn->originalZ;

				pawn->SetActorLocation(loc);
				pawn->tilePosition = from;
			}
			return false;
		}
	}

	// Everything is valid move pawn
	toSlot->holdedPawn = fromSlot->holdedPawn;
	toSlot->holdedPawn.tilePosition = to;
	toSlot->isHoldingPawn = true;

	fromSlot->holdedPawn = FCheckerPawnData();
	fromSlot->isHoldingPawn = false;

	if (pawn)
	{
		pawn->SetActorLocation(GetSlot(to)->worldPosition);
		pawn->tilePosition = to;
	}

	// Play sound
	if (HasAuthority())
	{
		Multicast_PlayMoveSound(GetSlot(to)->worldPosition);
	}

	// Multi capture
	if (captureMoves.Contains(to))
	{
		TArray<FIntPoint> possibleNextCapture = GetCaptureMoves(to);

		if (possibleNextCapture.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Multi-capture continues"));
			changeTurn = false;
		}
	}

	if (changeTurn)
	{
		TryPromotePawn(toSlot, pawn);

		ACheckerGameState* GS = GetWorld()->GetGameState<ACheckerGameState>();
		if (GS)
		{
			GS->whichTurn = (GS->whichTurn == ECheckerPawnType::White)
				? ECheckerPawnType::Black
				: ECheckerPawnType::White;
			GS->OnTurnChanged.Broadcast(GS->whichTurn);
		}
	}

	// Force replication update
	ForceNetUpdate();

	UE_LOG(LogTemp, Warning, TEXT("Moved from (%d,%d) to (%d,%d)"),
		from.X, from.Y, to.X, to.Y);

	return true;
}

void ABoard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABoard, boardState);
}