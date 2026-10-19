#include "ClockActor.h"

AClockActor::AClockActor()
{
	defaultSceneComponenet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Default Scene Component"));
	SetRootComponent(defaultSceneComponenet);

	whiteSecondsHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("White Seconds Hand"));
	whiteSecondsHand->SetupAttachment(RootComponent);
	whiteMinutesHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("White Minutes Hand"));
	whiteMinutesHand->SetupAttachment(RootComponent);
	blackSecondsHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Black Seconds Hand"));
	blackSecondsHand->SetupAttachment(RootComponent);
	blackMinutesHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Black Minutes Hand"));
	blackMinutesHand->SetupAttachment(RootComponent);

	whiteTapper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("White Tapper"));
	whiteTapper->SetupAttachment(RootComponent);

	blackTapper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Black Tapper"));
	blackTapper->SetupAttachment(RootComponent);
	
	PrimaryActorTick.bCanEverTick = true;
}

void AClockActor::BeginPlay()
{
	Super::BeginPlay();

	gameState = GetWorld()->GetGameState<ACheckerGameState>();

	// Set initial rotations of hands
	InitializeClock(gameState->whiteTime, whiteSecondsHand, whiteMinutesHand);
	InitializeClock(gameState->blackTime, blackSecondsHand, blackMinutesHand);

	// Set tappers initial positions
	if (whiteTapper)
	{
		whiteTapperStart = whiteTapper->GetRelativeLocation();
	}

	if (blackTapper)
	{
		blackTapperStart = blackTapper->GetRelativeLocation();
	}

	if (gameState)
	{
		gameState->OnTurnChanged.AddUObject(this, &AClockActor::HandleTurnChanged);
	}

	HandleTurnChanged(gameState->whichTurn);
}

void AClockActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateClock(gameState->whiteTime, whiteSecondsHand, whiteMinutesHand);
	UpdateClock(gameState->blackTime, blackSecondsHand, blackMinutesHand);
}

void AClockActor::InitializeClock(float timeInSeconds, UStaticMeshComponent* secondsHand, UStaticMeshComponent* minutesHand)
{
	if (!secondsHand || !minutesHand) return;

	secondsHand->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));

	const float minutesFullAngle = -30.f;

	minutesHand->SetRelativeRotation(FRotator(timeInSeconds / 60 * minutesFullAngle, 0.f, 0.f));
}

void AClockActor::UpdateClock(float timeLeft, UStaticMeshComponent* secondsHand, UStaticMeshComponent* minutesHand)
{
	if (!secondsHand || !minutesHand) return;

	// --- MINUTES ---
	const float minutesFullAngle = -30.f;

	float minutes = timeLeft / 60.f;

	float minutesAngle = minutes * minutesFullAngle;

	minutesHand->SetRelativeRotation(FRotator(minutesAngle, 0.f, 0.f));

	// --- SECONDS ---
	float seconds = FMath::Fmod(timeLeft, 60.f);

	float secondsAlpha = seconds / 60.f;

	float secondsAngle = -secondsAlpha * 360.f;

	secondsHand->SetRelativeRotation(FRotator(secondsAngle, 0.f, 0.f));
}

void AClockActor::HandleTurnChanged(ECheckerPawnType newTurn)
{
	if (!gameState || !whiteTapper || !blackTapper) return;

	// Reset both
	FVector whitePos = whiteTapperStart;
	FVector blackPos = blackTapperStart;

	if (gameState->whichTurn == ECheckerPawnType::White)
	{
		blackPos.Z += tapperPressedOffset; // move down
	}
	else
	{
		whitePos.Z += tapperPressedOffset; // move down
	}

	whiteTapper->SetRelativeLocation(whitePos);
	blackTapper->SetRelativeLocation(blackPos);
}