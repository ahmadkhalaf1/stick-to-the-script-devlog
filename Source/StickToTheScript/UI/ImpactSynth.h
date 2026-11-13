#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "ImpactSynth.generated.h"

UENUM()
enum class EImpactSound : uint8
{
	Punch,
	HeavyHit,
	BoneBreak,
	Knockout,
	Block,
	Whoosh,
	Clapper,
	CutSting,
	Gunshot,
	SpinWhoosh,
	Shout,
	Clang,
	Count
};

// Every sound in the game so far is made here from sine sweeps and filtered noise, so there is no audio asset to license.
UCLASS()
class UImpactSynth : public USynthComponent
{
	GENERATED_BODY()

public:
	void Trigger(EImpactSound Sound, float Volume = 1.f);

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	struct FVoice
	{
		int32 Sound = 0;
		int32 Position = 0;
		float Volume = 1.f;
	};

	TArray<float> Sounds[static_cast<int32>(EImpactSound::Count)];

	// Trigger runs on the game thread and OnGenerateAudio on the audio thread.
	FCriticalSection VoiceLock;
	TArray<FVoice> Voices;

	bool bReportedSound = false;
};
