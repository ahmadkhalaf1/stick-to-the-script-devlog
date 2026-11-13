#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include <atomic>
#include "FilmScore.generated.h"

class UAudioComponent;
class USoundBase;

UENUM()
enum class EScoreMood : uint8
{
	Silent,
	// A bare pulse under the shoot: kick, bass and a quiet hat.
	Fight,
	// The whole band, faster, for the film.
	Film
};

// The music, made in code like every other sound in the game: a drum machine, a bass and a lead. Nothing to license.
UCLASS()
class UFilmMusic : public USynthComponent
{
	GENERATED_BODY()

public:
	void SetMood(EScoreMood Mood);

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	void StartStep();

	// SetMood runs on the game thread and OnGenerateAudio on the audio thread.
	std::atomic<int32> WantedMood { 0 };

	int32 PlayingMood = 0;
	int32 Rate = 48000;
	int32 Step = -1;
	int32 SamplesLeftInStep = 0;
	float Gain = 0.f;

	float KickPhase = 0.f;
	float KickLevel = 0.f;
	float BassPhase = 0.f;
	float BassLevel = 0.f;
	float BassHz = 55.f;
	float HatLevel = 0.f;
	float LastHiss = 0.f;
	float SnareLevel = 0.f;
	float SnareBody = 0.f;
	float LeadPhase = 0.f;
	float LeadLevel = 0.f;
	float LeadHz = 220.f;
	uint32 HissState = 22222u;

	bool bReportedSound = false;
};

// Listens to the take and tells the music what kind of moment it is.
UCLASS()
class AFilmScore : public AActor
{
	GENERATED_BODY()

public:
	AFilmScore();

	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void HandleAction();

	UFUNCTION()
	void HandleCut(ECutReason Reason);

	UFUNCTION()
	void HandleTakeFinished(const FTake& Take);

	UFUNCTION()
	void HandleFinalCutStarted(const FTake& Take);

	UFUNCTION()
	void HandleFinalCutFinished();

	void SetMood(EScoreMood Mood);

	UPROPERTY()
	TObjectPtr<UFilmMusic> Music;

	// Recorded music, where Content/Audio has a track for the mood (M_Fight, M_Film). It then plays in place of the band made in code.
	UPROPERTY()
	TObjectPtr<UAudioComponent> Recorded;

	UPROPERTY()
	TObjectPtr<USoundBase> FightTrack;

	UPROPERTY()
	TObjectPtr<USoundBase> FilmTrack;
};
