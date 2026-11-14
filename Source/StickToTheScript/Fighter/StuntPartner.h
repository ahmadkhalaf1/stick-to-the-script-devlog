#pragma once

#include "CoreMinimal.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StuntPartner.generated.h"

// An opponent. It thinks for itself: it joins the fight, closes in, strikes, guards and backs off.
UCLASS()
class AStuntPartner : public AStickFighter
{
	GENERATED_BODY()

public:
	AStuntPartner();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Makes this opponent fight, in the manner of its kind.
	void StartThinking(EOpponentKind Kind, float InWakeDistance);

	// True in the last moments of a gunman's aim, when its line has stopped following the player.
	bool IsAboutToFire() const;

	// True once it has joined the fight, until it is put down.
	bool IsInTheFight() const { return Brain != EBrain::Asleep && !IsDown(); }
	bool IsBoss() const { return bBoss; }

	// Its place in the scene's list of opponents. The side and the lane it keeps come from it.
	UPROPERTY(EditAnywhere, Category = "Partner")
	int32 PartnerIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Partner")
	TObjectPtr<AStickFighter> PlayerFighter;

	// Where it comes to when it joins the fight. It waits at its home until then.
	UPROPERTY(EditAnywhere, Category = "Partner")
	FVector SpotLocation = FVector::ZeroVector;

private:
	enum class EBrain : uint8
	{
		Asleep,
		Entering,
		Waiting,
		Closing,
		Striking
	};

	UFUNCTION()
	void HandleNewTake(int32 TakeNumber);

	UFUNCTION()
	void HandleAction();

	UFUNCTION()
	void HandleCut(ECutReason Reason);

	UFUNCTION()
	void HandleTakeFinished(const FTake& Take);

	void Think(float DeltaSeconds);
	void ThinkWithAGun(float DeltaSeconds, const FVector& Mine, const FVector& Theirs);
	void Fire();
	void ShowLine(float DeltaSeconds);
	bool ShouldWake() const;
	int32 CountAttackers() const;

	bool bThinks = false;
	bool bFightLive = false;
	EBrain Brain = EBrain::Asleep;
	// Only a few opponents come at the player at once. This one is among them.
	bool bHasTurn = false;
	bool bSettled = false;
	bool bSawPlayerStrike = false;
	float RestSecondsLeft = 0.f;
	float GuardSecondsLeft = 0.f;

	// A gunman's aim, drawn so the player can see it coming, and for a moment its shot.
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> AimLine;

	bool bShoots = false;
	bool bBoss = false;
	float AimSecondsLeft = 0.f;
	float TracerSecondsLeft = 0.f;
	FVector TracerEnd = FVector::ZeroVector;

	float WakeDistance = 520.f;
	float PreferredRange = 95.f;
	float WindUpSeconds = 0.45f;
	float RestSeconds = 1.2f;
	float KickShare = 0.25f;
	float GuardShare = 0.15f;
	bool bBacksOff = false;
	// What it sometimes throws in place of its plain punch and its plain kick.
	EMove PunchSpecial = EMove::Punch;
	EMove KickSpecial = EMove::Kick;
	float SpecialShare = 0.f;
};
