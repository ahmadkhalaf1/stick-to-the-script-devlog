#include "StickToTheScript/Fighter/Captive.h"

#include "Engine/World.h"
#include "StickToTheScript/Core/TakeEvents.h"

void ACaptive::BeginPlay()
{
	Super::BeginPlay();

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnTakeStarted.AddDynamic(this, &ACaptive::HandleNewTake);
	Events->OnFighterDown.AddDynamic(this, &ACaptive::HandleFighterDown);
}

void ACaptive::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bReleased)
	{
		Guard(EMove::Block);
	}
}

void ACaptive::HandleNewTake(int32 TakeNumber)
{
	OpponentsDown = 0;
	bReleased = false;
}

void ACaptive::HandleFighterDown(AActor* Fighter, bool bPlayer)
{
	if (!bPlayer && ++OpponentsDown >= OpponentsToBeat)
	{
		bReleased = true;
	}
}
