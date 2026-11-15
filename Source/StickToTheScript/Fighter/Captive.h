#pragma once

#include "CoreMinimal.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "Captive.generated.h"

// Someone to rescue. It stands out of everyone's reach with its arms up in front of its face until the last opponent
// is down, then lets them drop. It used to crouch: the kid's model, a third of which is head, folded into a heap.
// Nobody can hit it: it is on the player's side and outside the arena.
UCLASS()
class ACaptive : public AStickFighter
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// How many opponents have to fall before it is free.
	int32 OpponentsToBeat = 0;

private:
	UFUNCTION()
	void HandleNewTake(int32 TakeNumber);

	UFUNCTION()
	void HandleFighterDown(AActor* Fighter, bool bPlayer);

	int32 OpponentsDown = 0;
	bool bReleased = false;
};
