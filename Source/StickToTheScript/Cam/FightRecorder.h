#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "FightRecorder.generated.h"

class AStickFighter;

// Keeps every fighter's pose on every tick of a take, and every event of it, failed takes included.
UCLASS()
class AFightRecorder : public AActor
{
	GENERATED_BODY()

public:
	AFightRecorder();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// In the order every frame keeps: the player first, then the stunt partners.
	void Watch(const TArray<AStickFighter*>& FightersInOrder);

	void BeginTake(int32 Number);
	void AddFrame(float TimeSeconds, const TArray<FFighterPose>& Poses);

	// The recording of the take the director just finished, completed with what only the director knows.
	const FTake& CloseTake(const FTake& FromDirector);

	// Stops the take being recorded and keeps its last seconds: the end of a take that was cut, fall included.
	void KeepOuttake();
	// The latest few of them, in the order they were shot. The film shows them after its slate.
	const TArray<FTake>& GetOuttakes() const { return Outtakes; }

	AStickFighter* GetFighter(int32 Index) const;

	// The loose props, in the order every frame keeps.
	void WatchProps(const TArray<class AThrownProp*>& PropsInOrder);
	class AThrownProp* GetProp(int32 Index) const;

private:
	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void HandleHit(AActor* Attacker, AActor* Victim, EMove Move);

	UFUNCTION()
	void HandleBlocked(AActor* Attacker, AActor* Defender);

	UFUNCTION()
	void HandleSwing(AActor* Attacker, EMove Move);

	UFUNCTION()
	void HandleCut(ECutReason Reason);

	UFUNCTION()
	void HandleTakeFinished(const FTake& Take);

	FRecordedEvent& AddEvent(ERecordedEventKind Kind);
	int32 IndexOf(const AActor* Fighter) const;
	float TakeSeconds() const;
	void StopAndReport();

	UPROPERTY()
	TArray<TObjectPtr<AStickFighter>> Fighters;

	UPROPERTY()
	TArray<TObjectPtr<class AThrownProp>> Props;

	FTake Recording;
	TArray<FTake> Outtakes;
	// A take that was cut is recorded on until this moment of it, so that the fall is kept. Negative when none is running out.
	float StopAtSeconds = -1.f;
	bool bRecording = false;
	float TakeStartSeconds = 0.f;
};
