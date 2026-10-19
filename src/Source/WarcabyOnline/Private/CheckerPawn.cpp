#include "CheckerPawn.h"
#include "BoardSlot.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

ACheckerPawn::ACheckerPawn()
{
	collider = CreateDefaultSubobject<UBoxComponent>(TEXT("Checker Box Collider"));
	SetRootComponent(collider);

	collider->SetBoxExtent(FVector(40.f, 40.f, 40.f));

	collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	collider->SetCollisionObjectType(ECC_WorldDynamic);

	collider->SetCollisionResponseToAllChannels(ECR_Ignore);
	collider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Overlaps only for slots
	collider->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);

	collider->SetGenerateOverlapEvents(true);

	checkerPawnMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Checker Pawn Mesh"));
	checkerPawnMesh->SetupAttachment(collider);

	// Mesh should not interfere with traces
	checkerPawnMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	crown = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Checker Crown"));
	crown->SetupAttachment(collider);
	crown->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void ACheckerPawn::BeginPlay()
{
	Super::BeginPlay();

	if (checkerPawnMesh)
	{
		dynamicMaterial = checkerPawnMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	originalZ = GetActorLocation().Z;
	destinationTilePosition = tilePosition;

	crown->SetVisibility(isKing);
}

void ACheckerPawn::SetColor(FLinearColor color)
{
	if (dynamicMaterial)
	{
		dynamicMaterial->SetVectorParameterValue(TEXT("Color"), color);
	}
}

void ACheckerPawn::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (OtherActor)
	{
		ABoardSlot* overlappedSlot = Cast<ABoardSlot>(OtherActor);

		if (overlappedSlot)
		{
			destinationTilePosition = overlappedSlot->tilePosition;
			overlappedSlot->SetColor(FLinearColor::Green);
		}
	}
}

void ACheckerPawn::NotifyActorEndOverlap(AActor* OtherActor)
{
	if (OtherActor)
	{
		ABoardSlot* overlappedSlot = Cast<ABoardSlot>(OtherActor);

		if (overlappedSlot)
		{
			//destinationTilePosition = overlappedSlot->tilePosition;
			overlappedSlot->SetColor(overlappedSlot->lastRememberedColor);
		}
	}
}

void ACheckerPawn::OnRep_PawnType()
{
	if (!dynamicMaterial && checkerPawnMesh)
	{
		dynamicMaterial = checkerPawnMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (dynamicMaterial)
	{
		FLinearColor color = (pawnType == ECheckerPawnType::White)
			? FLinearColor::White
			: FLinearColor::Black;

		dynamicMaterial->SetVectorParameterValue(TEXT("Color"), color);
	}
}

void ACheckerPawn::OnRep_IsKing()
{
	Promote();
}

void ACheckerPawn::Promote()
{
	if (!crown) return;
	
	crown->SetVisibility(isKing);
}

void ACheckerPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACheckerPawn, tilePosition);
	DOREPLIFETIME(ACheckerPawn, isKing);
	DOREPLIFETIME(ACheckerPawn, pawnType);
}