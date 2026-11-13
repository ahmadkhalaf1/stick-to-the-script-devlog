#include "StickToTheScript/UI/FilmScore.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "StickToTheScript/Core/StickGameInstance.h"
#include "StickToTheScript/UI/ImpactEffects.h"
#include "StickToTheScript/Core/TakeEvents.h"

DEFINE_LOG_CATEGORY_STATIC(LogFilmScore, Log, All);

namespace
{
	const float ScoreBeatsPerMinute[] = { 112.f, 112.f, 140.f };
	const float ScoreLoudness[] = { 0.f, 0.42f, 0.7f };
	const float ScoreFadeSeconds = 0.25f;

	// Four bars in A minor: A, A, F, G. The bass plays the root, the lead climbs the chord two octaves up.
	const float ScoreRoots[] = { 55.f, 55.f, 43.65f, 49.f };
	const float ScoreMinorChord[] = { 1.f, 1.189f, 1.498f, 2.f };
	const float ScoreMajorChord[] = { 1.f, 1.26f, 1.498f, 2.f };
	const int32 ScoreStepsInBar = 16;

	const float KickFadeSeconds = 0.12f;
	const float BassFadeSeconds = 0.11f;
	const float HatFadeSeconds = 0.02f;
	const float SnareFadeSeconds = 0.07f;
	const float LeadFadeSeconds = 0.09f;

	// What a level is multiplied by every sample so that it dies away over about the given time.
	float FadePerSample(float Seconds, int32 Rate)
	{
		return FMath::Exp(-1.f / (Seconds * Rate));
	}
}

void UFilmMusic::SetMood(EScoreMood Mood)
{
	WantedMood = static_cast<int32>(Mood);
}

bool UFilmMusic::Init(int32& SampleRate)
{
	NumChannels = 1;
	Rate = SampleRate;
	return true;
}

// One sixteenth note: decides which instruments sound on it.
void UFilmMusic::StartStep()
{
	Step = (Step + 1) % (ScoreStepsInBar * static_cast<int32>(UE_ARRAY_COUNT(ScoreRoots)));
	SamplesLeftInStep = FMath::RoundToInt32(Rate * 60.f / ScoreBeatsPerMinute[PlayingMood] / 4.f);

	const int32 Bar = Step / ScoreStepsInBar;
	const int32 InBar = Step % ScoreStepsInBar;
	const bool bFilm = PlayingMood == static_cast<int32>(EScoreMood::Film);

	if (InBar % 4 == 0)
	{
		KickLevel = 1.f;
		KickPhase = 0.f;
	}
	if (bFilm && InBar % 8 == 4)
	{
		SnareLevel = 1.f;
	}
	// The hat marks the off-beats under a fight, and every sixteenth in the film.
	if (bFilm || InBar % 4 == 2)
	{
		HatLevel = InBar % 4 == 2 ? 1.f : 0.5f;
	}
	if (InBar % 2 == 0)
	{
		const int32 Eighth = InBar / 2;
		BassHz = ScoreRoots[Bar] * (Eighth == 3 || Eighth == 6 ? 2.f : 1.f);
		BassLevel = 1.f;
	}
	if (bFilm)
	{
		const float* Chord = Bar < 2 ? ScoreMinorChord : ScoreMajorChord;
		// Up the chord in one bar and down it in the next.
		const int32 Note = Bar % 2 == 0 ? InBar % 4 : 3 - InBar % 4;
		LeadHz = ScoreRoots[Bar] * 4.f * Chord[Note];
		LeadLevel = 1.f;
	}
}

int32 UFilmMusic::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	const int32 Wanted = WantedMood;
	if (Wanted != PlayingMood)
	{
		PlayingMood = Wanted;
		// A new mood starts on the first beat of the first bar.
		Step = -1;
		SamplesLeftInStep = 0;
	}

	const float Loudness = ScoreLoudness[PlayingMood];
	if (Loudness <= 0.f && Gain < 0.001f)
	{
		FMemory::Memzero(OutAudio, NumSamples * sizeof(float));
		return NumSamples;
	}

	const float KickFade = FadePerSample(KickFadeSeconds, Rate);
	const float BassFade = FadePerSample(BassFadeSeconds, Rate);
	const float HatFade = FadePerSample(HatFadeSeconds, Rate);
	const float SnareFade = FadePerSample(SnareFadeSeconds, Rate);
	const float LeadFade = FadePerSample(LeadFadeSeconds, Rate);
	const float GainStep = 1.f / (ScoreFadeSeconds * Rate);

	float Peak = 0.f;
	for (int32 Index = 0; Index < NumSamples; ++Index)
	{
		if (Loudness > 0.f && --SamplesLeftInStep < 0)
		{
			StartStep();
		}

		// The kick is a sine that drops in pitch as it dies.
		KickPhase += UE_TWO_PI * (48.f + 90.f * KickLevel * KickLevel) / Rate;
		float Sample = FMath::Sin(KickPhase) * KickLevel * 0.9f;
		KickLevel *= KickFade;

		// The bass is a sine pushed until it squares off.
		BassPhase += UE_TWO_PI * BassHz / Rate;
		if (BassPhase > UE_TWO_PI)
		{
			BassPhase -= UE_TWO_PI;
		}
		Sample += FMath::Tanh(FMath::Sin(BassPhase) * 3.f) * BassLevel * 0.38f;
		BassLevel *= BassFade;

		HissState = HissState * 1664525u + 1013904223u;
		const float Hiss = static_cast<float>(HissState >> 8) * (2.f / 16777216.f) - 1.f;

		// The hat is only the fast part of the hiss, the snare its slower body.
		Sample += (Hiss - LastHiss) * HatLevel * 0.12f;
		LastHiss = Hiss;
		HatLevel *= HatFade;

		SnareBody += 0.45f * (Hiss - SnareBody);
		Sample += SnareBody * SnareLevel * 0.42f;
		SnareLevel *= SnareFade;

		// The lead is a thin pulse wave.
		LeadPhase += LeadHz / Rate;
		LeadPhase -= FMath::FloorToFloat(LeadPhase);
		Sample += (LeadPhase < 0.25f ? 1.f : -0.33f) * LeadLevel * 0.1f;
		LeadLevel *= LeadFade;

		Gain = FMath::Clamp(Gain + (Loudness > Gain ? GainStep : -GainStep), 0.f, FMath::Max(Loudness, Gain));
		OutAudio[Index] = FMath::Tanh(Sample * Gain);
		Peak = FMath::Max(Peak, FMath::Abs(OutAudio[Index]));
	}

	if (!bReportedSound && Peak > 0.05f)
	{
		bReportedSound = true;
		UE_LOG(LogFilmScore, Log, TEXT("First audible music sent to the mixer, peak %.2f."), Peak);
	}
	return NumSamples;
}

AFilmScore::AFilmScore()
{
	Music = CreateDefaultSubobject<UFilmMusic>("Music");
	RootComponent = Music;
	// Music is not somewhere in the room, and slow motion must not bend its pitch the way it bends the blows.
	Music->bAllowSpatialization = false;
	Music->bIsUISound = true;

	Recorded = CreateDefaultSubobject<UAudioComponent>("Recorded");
	Recorded->SetupAttachment(RootComponent);
	Recorded->bAutoActivate = false;
	Recorded->bAllowSpatialization = false;
	Recorded->bIsUISound = true;
	// Under the blows, not over them.
	Recorded->VolumeMultiplier = 0.55f;
}

void AFilmScore::BeginPlay()
{
	Super::BeginPlay();

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnTakeStarted.AddDynamic(this, &AFilmScore::HandleTakeStarted);
	Events->OnAction.AddDynamic(this, &AFilmScore::HandleAction);
	Events->OnCut.AddDynamic(this, &AFilmScore::HandleCut);
	Events->OnTakeFinished.AddDynamic(this, &AFilmScore::HandleTakeFinished);
	Events->OnFinalCutStarted.AddDynamic(this, &AFilmScore::HandleFinalCutStarted);
	Events->OnFinalCutFinished.AddDynamic(this, &AFilmScore::HandleFinalCutFinished);

	// Each scene can have fight music of its own. One without takes the track that has no number.
	const UStickGameInstance* Game = Cast<UStickGameInstance>(GetGameInstance());
	FightTrack = AImpactEffects::FindRecording(FString::Printf(TEXT("M_Fight_%d"), Game ? Game->SceneIndex + 1 : 1));
	if (!FightTrack)
	{
		FightTrack = AImpactEffects::FindRecording(TEXT("M_Fight"));
	}
	FilmTrack = AImpactEffects::FindRecording(TEXT("M_Film"));
	Music->Start();
}

void AFilmScore::SetMood(EScoreMood Mood)
{
	USoundBase* Track = Mood == EScoreMood::Fight ? FightTrack.Get() : Mood == EScoreMood::Film ? FilmTrack.Get() : nullptr;
	Music->SetMood(Track ? EScoreMood::Silent : Mood);
	if (Track)
	{
		Recorded->SetSound(Track);
		Recorded->FadeIn(0.25f);
	}
	else if (Recorded->IsPlaying())
	{
		Recorded->FadeOut(0.25f, 0.f);
	}
}

void AFilmScore::HandleTakeStarted(int32 TakeNumber)
{
	SetMood(EScoreMood::Silent);
}

void AFilmScore::HandleAction()
{
	SetMood(EScoreMood::Fight);
}

void AFilmScore::HandleCut(ECutReason Reason)
{
	SetMood(EScoreMood::Silent);
}

void AFilmScore::HandleTakeFinished(const FTake& Take)
{
	SetMood(EScoreMood::Silent);
}

void AFilmScore::HandleFinalCutStarted(const FTake& Take)
{
	SetMood(EScoreMood::Film);
}

void AFilmScore::HandleFinalCutFinished()
{
	SetMood(EScoreMood::Silent);
}
