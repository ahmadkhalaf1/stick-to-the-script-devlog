#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "ImpactEffects.generated.h"

class UImpactSynth;
class UMaterialInterface;
class USoundBase;
enum class EImpactSound : uint8;
class UStaticMesh;
class UStaticMeshComponent;

// What a strike does to the screen and the speakers: blood from where it landed, sparks off a guard, a frozen frame, slow motion on a knockout, and a sound for each.
// It only listens to the take events, so a replay that fires the same events gets the same effects.
// A sound is made in code unless there is a recording of it in Content/Audio, which is then played instead.
UCLASS()
class AImpactEffects : public AActor
{
	GENERATED_BODY()

public:
	AImpactEffects();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// The recording by that name in Content/Audio, or nothing. Tools/build_audio.py says how they are named.
	static USoundBase* FindRecording(const FString& Name);

private:
	struct FDroplet
	{
		FVector Velocity = FVector::ZeroVector;
		float Size = 0.f;
		float FloorHeight = 0.f;
		float SecondsLeft = 0.f;
		bool bFlying = false;
		bool bLeavesMark = false;
	};

	UFUNCTION()
	void HandleHit(AActor* Attacker, AActor* Victim, EMove Move);

	UFUNCTION()
	void HandleBlocked(AActor* Attacker, AActor* Defender);

	UFUNCTION()
	void HandleSwing(AActor* Attacker, EMove Move);

	UFUNCTION()
	void HandleBarGrabbed(AActor* Fighter);

	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void HandleCut(ECutReason Reason);

	UFUNCTION()
	void HandleFinalCutStarted(const FTake& Take);

	UFUNCTION()
	void HandleAction();

	UFUNCTION()
	void HandleTakeFinished(const FTake& Take);

	void Play(EImpactSound Sound, float Volume = 1.f);
	bool PlayRecording(FName Name, float Volume, float PitchSpread, bool bSlowsWithTheGame);
	void FindRecordings(FName Name);

	void Spray(const FVector& From, const FVector& Direction, int32 Count, const FLinearColor& Color, bool bLeavesMarks, float FloorHeight);
	void SlowTime(float Dilation, float RealSeconds);

	UPROPERTY()
	TObjectPtr<UImpactSynth> Synth;

	// Every take of every recorded sound. One of a sound's takes is picked each time, so a row of punches is not one sample.
	UPROPERTY()
	TArray<TObjectPtr<USoundBase>> Recordings;

	// Where a sound's takes are in Recordings: X is the first, Y how many.
	TMap<FName, FIntPoint> RecordingsByName;

	UPROPERTY()
	TObjectPtr<UStaticMesh> DropletMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> DropletMaterial;

	// A fixed pool, reused oldest first, so marks on the floor last for a while and the count never grows.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> DropletMeshes;

	TArray<FDroplet> Droplets;
	int32 NextDroplet = 0;
	float SlowRealSecondsLeft = 0.f;
	// The film does not replay a swing, only what landed, so a shot that lands there has to bring its own bang.
	bool bFilm = false;
	// Which arms each fighter had broken at its last hit, so the crack sounds once, when one breaks.
	TMap<TWeakObjectPtr<AActor>, uint8> BrokenArmsHeard;
};
