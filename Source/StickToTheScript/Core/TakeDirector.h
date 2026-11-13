#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "TakeDirector.generated.h"

class UTakeEvents;

UENUM()
enum class ETakePhase : uint8
{
	Idle,
	LeadIn,
	Fighting,
	Wrap,
	Cut,
	Finished
};

// The referee of a take: a lead-in, then the fight. The take is a wrap when every opponent is down, and a cut
// when the player is. Nobody judges the moves.
UCLASS()
class ATakeDirector : public AActor
{
	GENERATED_BODY()

public:
	ATakeDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void FighterWentDown(AActor* Fighter, bool bPlayer);

	// True while the game plays itself: the console variable sts.Autoplay.
	static bool IsAutoplayOn();

	// How many opponents the player has to put down.
	int32 OpponentCount = 0;

	void StartTake();
	void Advance(float DeltaSeconds);

	ETakePhase GetPhase() const { return Phase; }
	int32 GetTakeNumber() const { return Take.Number; }

	UPROPERTY(EditAnywhere, Category = "Take")
	FSceneScript Script;

	UPROPERTY(EditAnywhere, Category = "Take")
	float LeadInSeconds = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Take")
	float RetakeDelaySeconds = 2.5f;

	// How long the take runs on after the last opponent falls.
	UPROPERTY(EditAnywhere, Category = "Take")
	float WrapSeconds = 1.8f;

private:
	UTakeEvents& Events() const;
	void EnterPhase(ETakePhase NewPhase, float Seconds);
	void FinishTake();

	ETakePhase Phase = ETakePhase::Idle;
	float PhaseSecondsLeft = 0.f;
	int32 OpponentsDown = 0;
	FTake Take;
};
