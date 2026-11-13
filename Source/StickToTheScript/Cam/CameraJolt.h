#pragma once

#include "CoreMinimal.h"

// The shake an impact gives a camera: strong at once, then fading. Shared by every camera that reacts to hits.
struct FCameraJolt
{
	void Add(float Strength)
	{
		Amount = FMath::Max(Amount, Strength);
	}

	// Returns the rotation to add to the camera this frame.
	FRotator Advance(float DeltaSeconds, float TimeSeconds, float RestingDegrees, float JoltDegrees)
	{
		Amount = FMath::Max(Amount - DeltaSeconds * FadePerSecond, 0.f);

		const float Degrees = RestingDegrees + JoltDegrees * Amount * Amount;
		const float Time = TimeSeconds * (1.5f + 14.f * Amount);
		return FRotator(
			FMath::PerlinNoise1D(Time) * Degrees,
			FMath::PerlinNoise1D(Time + 50.f) * Degrees,
			FMath::PerlinNoise1D(Time + 100.f) * Degrees * 0.6f);
	}

	float Amount = 0.f;
	float FadePerSecond = 2.5f;
};
