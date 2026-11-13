#include "StickToTheScript/UI/ImpactSynth.h"

DEFINE_LOG_CATEGORY_STATIC(LogImpactSynth, Log, All);

namespace
{
	const float MasterGain = 0.7f;

	struct FNoise
	{
		float Next()
		{
			State = State * 1664525u + 1013904223u;
			return static_cast<float>(State >> 8) * (2.f / 16777216.f) - 1.f;
		}

		uint32 State = 22222u;
	};

	void Reserve(TArray<float>& Out, int32 Count)
	{
		if (Out.Num() < Count)
		{
			Out.SetNumZeroed(Count);
		}
	}

	// A sine whose pitch falls while it fades: the body of a thud.
	void AddThump(TArray<float>& Out, int32 Rate, float StartHz, float EndHz, float Seconds, float Gain)
	{
		const int32 Count = FMath::RoundToInt32(Seconds * Rate);
		Reserve(Out, Count);

		float Phase = 0.f;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Progress = static_cast<float>(Index) / Count;
			Phase += UE_TWO_PI * FMath::Lerp(StartHz, EndHz, Progress) / Rate;
			Out[Index] += FMath::Sin(Phase) * FMath::Square(1.f - Progress) * Gain;
		}
	}

	// Noise through a one-pole filter, dying away fast: the slap, crack or click on top of a thud. Brightness is 0 (dull) to 1 (sharp).
	void AddBurst(TArray<float>& Out, int32 Rate, FNoise& Noise, float StartSeconds, float Seconds, float Brightness, float Gain)
	{
		const int32 Start = FMath::RoundToInt32(StartSeconds * Rate);
		const int32 Count = FMath::RoundToInt32(Seconds * Rate);
		Reserve(Out, Start + Count);

		float Filtered = 0.f;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Filtered += Brightness * (Noise.Next() - Filtered);
			Out[Start + Index] += Filtered * FMath::Square(1.f - static_cast<float>(Index) / Count) * Gain;
		}
	}

	// Noise that swells and fades while it brightens: air moved by a fast limb.
	void AddWhoosh(TArray<float>& Out, int32 Rate, FNoise& Noise, float Seconds, float Gain)
	{
		const int32 Count = FMath::RoundToInt32(Seconds * Rate);
		Reserve(Out, Count);

		float Filtered = 0.f;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Progress = static_cast<float>(Index) / Count;
			Filtered += FMath::Lerp(0.04f, 0.4f, Progress) * (Noise.Next() - Filtered);
			Out[Index] += Filtered * FMath::Square(FMath::Sin(UE_PI * Progress)) * Gain;
		}
	}
}

namespace
{
	// A voice of sorts: a buzz whose pitch falls, with its loudest overtones where an open "ha" has them.
	void AddShout(TArray<float>& Out, int32 Rate, FNoise& Noise, float StartHz, float EndHz, float Seconds, float Gain)
	{
		const int32 Count = FMath::RoundToInt32(Seconds * Rate);
		Reserve(Out, Count);

		float Phase = 0.f;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Progress = static_cast<float>(Index) / Count;
			const float Hz = FMath::Lerp(StartHz, EndHz, Progress);
			Phase += UE_TWO_PI * Hz / Rate;

			float Sample = 0.f;
			for (int32 Overtone = 1; Overtone <= 10; ++Overtone)
			{
				const float At = Hz * Overtone;
				const float Vowel = FMath::Exp(-FMath::Square((At - 800.f) / 260.f)) + 0.7f * FMath::Exp(-FMath::Square((At - 1250.f) / 300.f)) + 0.12f;
				Sample += FMath::Sin(Phase * Overtone) * Vowel / Overtone;
			}
			const float Envelope = FMath::Min(Progress * 18.f, 1.f) * FMath::Square(1.f - Progress);
			Out[Index] += (Sample + Noise.Next() * 0.08f) * Envelope * Gain;
		}
	}

	// Struck metal: a handful of pitches that do not belong to one note, each dying at its own rate.
	void AddRing(TArray<float>& Out, int32 Rate, float Seconds, float Gain)
	{
		const int32 Count = FMath::RoundToInt32(Seconds * Rate);
		Reserve(Out, Count);

		const float Pitches[] = { 540.f, 1310.f, 2170.f, 3390.f };
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float At = static_cast<float>(Index) / Rate;
			float Sample = 0.f;
			for (int32 Pitch = 0; Pitch < UE_ARRAY_COUNT(Pitches); ++Pitch)
			{
				Sample += FMath::Sin(UE_TWO_PI * Pitches[Pitch] * At) * FMath::Exp(-At * (6.f + 4.f * Pitch)) / (Pitch + 1);
			}
			Out[Index] += Sample * Gain;
		}
	}
}

void UImpactSynth::Trigger(EImpactSound Sound, float Volume)
{
	FScopeLock Lock(&VoiceLock);
	FVoice& Voice = Voices.AddDefaulted_GetRef();
	Voice.Sound = static_cast<int32>(Sound);
	Voice.Volume = Volume;
}

bool UImpactSynth::Init(int32& SampleRate)
{
	NumChannels = 1;

	FNoise Noise;
	auto Sound = [this](EImpactSound Which) -> TArray<float>& { return Sounds[static_cast<int32>(Which)]; };
	for (TArray<float>& Samples : Sounds)
	{
		Samples.Reset();
	}

	AddThump(Sound(EImpactSound::Punch), SampleRate, 160.f, 55.f, 0.14f, 0.9f);
	AddBurst(Sound(EImpactSound::Punch), SampleRate, Noise, 0.f, 0.06f, 0.35f, 0.8f);

	AddThump(Sound(EImpactSound::HeavyHit), SampleRate, 110.f, 38.f, 0.3f, 1.f);
	AddBurst(Sound(EImpactSound::HeavyHit), SampleRate, Noise, 0.f, 0.09f, 0.5f, 0.9f);

	for (const float ClickAt : { 0.f, 0.028f, 0.05f, 0.095f })
	{
		AddBurst(Sound(EImpactSound::BoneBreak), SampleRate, Noise, ClickAt, 0.014f, 0.95f, 0.8f);
	}
	AddThump(Sound(EImpactSound::BoneBreak), SampleRate, 700.f, 240.f, 0.07f, 0.35f);

	AddThump(Sound(EImpactSound::Knockout), SampleRate, 90.f, 28.f, 0.6f, 1.f);
	AddBurst(Sound(EImpactSound::Knockout), SampleRate, Noise, 0.f, 0.15f, 0.3f, 0.9f);
	AddBurst(Sound(EImpactSound::Knockout), SampleRate, Noise, 0.05f, 0.3f, 0.08f, 0.5f);

	AddThump(Sound(EImpactSound::Block), SampleRate, 420.f, 260.f, 0.06f, 0.5f);
	AddBurst(Sound(EImpactSound::Block), SampleRate, Noise, 0.f, 0.03f, 0.7f, 0.6f);

	AddWhoosh(Sound(EImpactSound::Whoosh), SampleRate, Noise, 0.2f, 0.35f);

	AddBurst(Sound(EImpactSound::Clapper), SampleRate, Noise, 0.f, 0.02f, 0.95f, 0.9f);
	AddThump(Sound(EImpactSound::Clapper), SampleRate, 900.f, 700.f, 0.03f, 0.4f);

	AddThump(Sound(EImpactSound::CutSting), SampleRate, 300.f, 140.f, 0.4f, 0.7f);

	AddBurst(Sound(EImpactSound::Gunshot), SampleRate, Noise, 0.f, 0.05f, 0.95f, 1.f);
	AddThump(Sound(EImpactSound::Gunshot), SampleRate, 190.f, 55.f, 0.13f, 0.9f);
	AddBurst(Sound(EImpactSound::Gunshot), SampleRate, Noise, 0.03f, 0.28f, 0.12f, 0.45f);

	AddWhoosh(Sound(EImpactSound::SpinWhoosh), SampleRate, Noise, 0.4f, 0.5f);
	AddShout(Sound(EImpactSound::Shout), SampleRate, Noise, 470.f, 330.f, 0.22f, 0.5f);
	AddRing(Sound(EImpactSound::Clang), SampleRate, 0.6f, 0.45f);
	AddBurst(Sound(EImpactSound::Clang), SampleRate, Noise, 0.f, 0.012f, 0.9f, 0.5f);
	return true;
}

int32 UImpactSynth::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	FMemory::Memzero(OutAudio, NumSamples * sizeof(float));

	FScopeLock Lock(&VoiceLock);
	for (int32 VoiceIndex = Voices.Num() - 1; VoiceIndex >= 0; --VoiceIndex)
	{
		FVoice& Voice = Voices[VoiceIndex];
		const TArray<float>& Samples = Sounds[Voice.Sound];
		const int32 Count = FMath::Min(NumSamples, Samples.Num() - Voice.Position);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			OutAudio[Index] += Samples[Voice.Position + Index] * Voice.Volume;
		}

		Voice.Position += Count;
		if (Voice.Position >= Samples.Num())
		{
			Voices.RemoveAtSwap(VoiceIndex);
		}
	}

	float Peak = 0.f;
	// Several sounds can land on the same instant. This keeps their sum from clipping harshly.
	for (int32 Index = 0; Index < NumSamples; ++Index)
	{
		OutAudio[Index] = FMath::Tanh(OutAudio[Index] * MasterGain);
		Peak = FMath::Max(Peak, FMath::Abs(OutAudio[Index]));
	}

	if (!bReportedSound && Peak > 0.05f)
	{
		bReportedSound = true;
		UE_LOG(LogImpactSynth, Log, TEXT("First audible buffer sent to the mixer, peak %.2f."), Peak);
	}
	return NumSamples;
}
