#include "BoardSlot.h"
#include "Net/UnrealNetwork.h"

ABoardSlot::ABoardSlot()
{
	rootSlotComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Slot Component"));
	SetRootComponent(rootSlotComponent);

	shadowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shadow Mesh"));
	shadowMesh->SetupAttachment(rootSlotComponent);

	// VERY IMPORTANT
	shadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	boxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Collider"));
	boxCollider->SetupAttachment(rootSlotComponent);

	boxCollider->SetBoxExtent(FVector(50.f, 50.f, 20.f));

	boxCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	boxCollider->SetCollisionObjectType(ECC_WorldStatic);

	boxCollider->SetCollisionResponseToAllChannels(ECR_Ignore);

	// Only overlap with pawn
	boxCollider->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	boxCollider->SetGenerateOverlapEvents(true);

	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void ABoardSlot::BeginPlay()
{
	if (shadowMesh)
	{
		dynamicMaterial = shadowMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
}

void ABoardSlot::SetHighlight(bool isHiglighted)
{
	if(shadowMesh)
	{
		shadowMesh->SetVisibility(isHiglighted);
	}
}

FLinearColor ABoardSlot::GetColor()
{
	if (!dynamicMaterial) return FLinearColor::Black;

	FLinearColor color;
	dynamicMaterial->GetVectorParameterValue(TEXT("Color"), color);

	return color;
}

void ABoardSlot::SetColor(FLinearColor color)
{
	lastRememberedColor = GetColor();

	if(dynamicMaterial)
	{
		dynamicMaterial->SetVectorParameterValue(TEXT("Color"), color);
	}
}

void ABoardSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABoardSlot, tilePosition);
}