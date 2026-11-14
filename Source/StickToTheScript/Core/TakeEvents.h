#pragma once

#include "CoreMinimal.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "TakeEvents.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTakeStartedSignature, int32, TakeNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHitSignature, AActor*, Attacker, AActor*, Victim, EMove, Move);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBlockedSignature, AActor*, Attacker, AActor*, Defender);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCutSignature, ECutReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTakeFinishedSignature, const FTake&, Take);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFinalCutStartedSignature, const FTake&, Take);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFinalCutFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCardSignature, const FText&, Headline, const FText&, Detail);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPayChangedSignature, int32, Pay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBoxOfficeSignature, const FText&, What, int32, Dollars);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWardrobeSignature, const FText&, Choices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSwingSignature, AActor*, Attacker, EMove, Move);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFighterDownSignature, AActor*, Fighter, bool, bPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBarGrabbedSignature, AActor*, Fighter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOuttakeSignature, int32, TakeNumber, bool, bCut);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFinishSignature, AActor*, Attacker, int32, Beat);

UCLASS()
class UTakeEvents : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Take")
	FTakeStartedSignature OnTakeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Take")
	FHitSignature OnHit;

	UPROPERTY(BlueprintAssignable, Category = "Take")
	FBlockedSignature OnBlocked;

	UPROPERTY(BlueprintAssignable, Category = "Take")
	FCutSignature OnCut;

	UPROPERTY(BlueprintAssignable, Category = "Take")
	FTakeFinishedSignature OnTakeFinished;

	UPROPERTY(BlueprintAssignable, Category = "Take")
	FFinalCutStartedSignature OnFinalCutStarted;

	UPROPERTY(BlueprintAssignable, Category = "Take")
	FFinalCutFinishedSignature OnFinalCutFinished;

	// A full-screen card: title, scene, pay, results. An empty headline takes the card down.
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FCardSignature OnCard;

	UPROPERTY(BlueprintAssignable, Category = "Take")
	FPayChangedSignature OnPayChanged;

	// Something the player did in the shoot earned money at the box office, or earned nothing because it was seen too often.
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FBoxOfficeSignature OnBoxOffice;

	// The wardrobe is open and these are the choices as they stand, one per line. Empty text closes it.
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FWardrobeSignature OnWardrobe;

	// The fight is live: the lead-in is over and everyone may move.
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FActionSignature OnAction;

	// A strike has started. It may still miss.
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FSwingSignature OnSwing;

	// A fighter was knocked out.
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FFighterDownSignature OnFighterDown;

	// A fighter caught hold of a bar overhead.
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FBarGrabbedSignature OnBarGrabbed;

	// After the film, the end of a take that was cut is shown again. Said as it starts, and once more at its "Cut!".
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FOuttakeSignature OnOuttake;

	// A blow of the big finish has been started: three of them on one opponent, counted from 0, and the last takes
	// everyone near with it. Each one's hit follows as OnHit.
	UPROPERTY(BlueprintAssignable, Category = "Take")
	FFinishSignature OnFinish;

private:
	UFUNCTION()
	void LogFinish(AActor* Attacker, int32 Beat);

	UFUNCTION()
	void LogTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void LogHit(AActor* Attacker, AActor* Victim, EMove Move);

	UFUNCTION()
	void LogBlocked(AActor* Attacker, AActor* Defender);

	UFUNCTION()
	void LogCut(ECutReason Reason);

	UFUNCTION()
	void LogTakeFinished(const FTake& Take);

	UFUNCTION()
	void LogFinalCutStarted(const FTake& Take);

	UFUNCTION()
	void LogFinalCutFinished();

	UFUNCTION()
	void LogCard(const FText& Headline, const FText& Detail);

	UFUNCTION()
	void LogPayChanged(int32 Pay);

	UFUNCTION()
	void LogBoxOffice(const FText& What, int32 Dollars);

	UFUNCTION()
	void LogWardrobe(const FText& Choices);

	UFUNCTION()
	void LogAction();

	UFUNCTION()
	void LogSwing(AActor* Attacker, EMove Move);

	UFUNCTION()
	void LogFighterDown(AActor* Fighter, bool bPlayer);

	UFUNCTION()
	void LogBarGrabbed(AActor* Fighter);

	UFUNCTION()
	void LogOuttake(int32 TakeNumber, bool bCut);
};
