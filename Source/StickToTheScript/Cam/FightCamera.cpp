#include "StickToTheScript/Cam/FightCamera.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"

namespace
{
	const float PushInDegrees = 5.f;
	const float ShakeDegrees = 2.2f;

	const float JoltOnPunch = 0.55f;
	const float JoltOnHeavyHit = 1.f;
	const float JoltOnBlock = 0.3f;

	const float FollowSpeed = 3.5f;

	// An ordinary lens three metres from a figure blurs almost nothing behind it. The film's lens gets a sensor three
	// times as wide and is opened right up, which is what makes a miniature look like one.
	const float FilmSensorWidth = 70.f;
	const float FilmFstop = 2.f;
}

AFightCamera::AFightCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("Mount");
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(BaseFieldOfView);
	Camera->bConstrainAspectRatio = false;
}

void AFightCamera::BeginPlay()
{
	Super::BeginPlay();

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnHit.AddDynamic(this, &AFightCamera::HandleHit);
	Events->OnBlocked.AddDynamic(this, &AFightCamera::HandleBlocked);
	Events->OnTakeStarted.AddDynamic(this, &AFightCamera::HandleTakeStarted);
}

void AFightCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Subject)
	{
		// The camera slides along its own sideways axis, so it works whichever way a stage is laid out.
		const FVector Right = GetActorRightVector();
		const float Wanted = static_cast<float>((Subject->GetJointPositions()[StickJoint::PelvisJoint] - BaseLocation) | Right);
		Along = bSnapToSubject ? Wanted : FMath::FInterpTo(Along, Wanted, DeltaSeconds, FollowSpeed);
		bSnapToSubject = false;

		SetActorLocation(BaseLocation + Right * Along);
	}

	Camera->SetRelativeRotation(Jolt.Advance(DeltaSeconds, GetWorld()->GetTimeSeconds(), 0.f, ShakeDegrees));
	Camera->SetFieldOfView(BaseFieldOfView - PushInDegrees * Jolt.Amount);
}

void AFightCamera::Follow(AStickFighter* Fighter)
{
	Subject = Fighter;
	BaseLocation = GetActorLocation();
	bSnapToSubject = true;
}

void AFightCamera::FocusOn(const FVector& Point)
{
	FPostProcessSettings& Lens = Camera->PostProcessSettings;
	Lens.bOverride_DepthOfFieldFocalDistance = true;
	Lens.bOverride_DepthOfFieldFstop = true;
	Lens.bOverride_DepthOfFieldSensorWidth = true;
	Lens.DepthOfFieldFocalDistance = static_cast<float>(FVector::Dist(Camera->GetComponentLocation(), Point));
	Lens.DepthOfFieldFstop = FilmFstop;
	Lens.DepthOfFieldSensorWidth = FilmSensorWidth;
}

// A retake puts the fighters back at the start, and the camera cuts back with them instead of gliding.
void AFightCamera::HandleTakeStarted(int32 TakeNumber)
{
	bSnapToSubject = true;
}

void AFightCamera::HandleHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	Jolt.Add(IsHeavyMove(Move) ? JoltOnHeavyHit : JoltOnPunch);
}

void AFightCamera::HandleBlocked(AActor* Attacker, AActor* Defender)
{
	Jolt.Add(JoltOnBlock);
}
