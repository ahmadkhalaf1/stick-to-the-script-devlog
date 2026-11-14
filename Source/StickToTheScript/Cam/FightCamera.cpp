#include "StickToTheScript/Cam/FightCamera.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StickToTheScript/Fighter/ThrownProp.h"

namespace
{
	const float PushInDegrees = 5.f;
	const float ShakeDegrees = 2.2f;

	const float JoltOnPunch = 0.8f;
	const float JoltOnHeavyHit = 1.f;
	const float JoltOnBlock = 0.3f;

	const float FollowSpeed = 3.5f;

	// A gun reaches three times as far as the picture is wide. These match how the fighter itself aims one: at the
	// nearest opponent this close and roughly ahead.
	const double AimedAtWithin = 950.;
	const double AimedAtCosine = 0.5;
	const double GunInHandWithin = 70.;
	// As far back as the camera goes to hold both. The sets' walls and lamp stands are further back than this.
	const float MostPullBack = 380.f;
	const double FramingMargin = 140.;
	const float FramingSpeed = 5.f;
	const float KeepAimedSeconds = 0.7f;

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
		const FVector Forward = GetActorForwardVector();
		const FVector Pelvis = Subject->GetJointPositions()[StickJoint::PelvisJoint];
		const float Now = GetWorld()->GetTimeSeconds();
		FVector Found;
		if (FindAimedAt(Found))
		{
			AimedAt = Found;
			AimedUntilSeconds = Now + KeepAimedSeconds;
		}

		const bool bFraming = Now < AimedUntilSeconds;
		float Wanted = static_cast<float>((Pelvis - BaseLocation) | Right);
		float WantedPullBack = 0.f;
		if (bFraming)
		{
			const double HalfApart = ((AimedAt - Pelvis) | Right) * 0.5;
			// The nearer of the two is the one the picture's edge reaches first.
			const double Depth = FMath::Min((Pelvis - BaseLocation) | Forward, (AimedAt - BaseLocation) | Forward);
			const double TanHalf = FMath::Tan(FMath::DegreesToRadians(BaseFieldOfView * 0.5));
			WantedPullBack = static_cast<float>(FMath::Clamp((FMath::Abs(HalfApart) + FramingMargin) / TanHalf - Depth, 0., static_cast<double>(MostPullBack)));
			// Where even that is not far enough back, the one with the gun stays in the picture and the far end is what is lost.
			const double HalfSeen = FMath::Max((Depth + WantedPullBack) * TanHalf - FramingMargin, 0.);
			Wanted += static_cast<float>(FMath::Clamp(HalfApart, -HalfSeen, HalfSeen));
		}
		Along = bSnapToSubject ? Wanted : FMath::FInterpTo(Along, Wanted, DeltaSeconds, bFraming ? FramingSpeed : FollowSpeed);
		PullBack = bSnapToSubject ? WantedPullBack : FMath::FInterpTo(PullBack, WantedPullBack, DeltaSeconds, FramingSpeed);
		bSnapToSubject = false;

		SetActorLocation(BaseLocation + Right * Along - Forward * PullBack);
	}

	Camera->SetRelativeRotation(Jolt.Advance(DeltaSeconds, GetWorld()->GetTimeSeconds(), 0.f, ShakeDegrees));
	Camera->SetFieldOfView(BaseFieldOfView - PushInDegrees * Jolt.Amount);
}

bool AFightCamera::FindAimedAt(FVector& OutPoint) const
{
	const TArray<FVector> Joints = Subject->GetJointPositions();
	const FVector Hands = (Joints[StickJoint::LeftHand] + Joints[StickJoint::RightHand]) * 0.5;
	bool bGunInHands = false;
	for (TActorIterator<AThrownProp> It(GetWorld()); It && !bGunInHands; ++It)
	{
		bGunInHands = It->IsGun() && FVector::Dist(It->GetActorLocation(), Hands) < GunInHandWithin;
	}
	if (!bGunInHands)
	{
		return false;
	}

	const FVector Pelvis = Joints[StickJoint::PelvisJoint];
	const FVector Aim = ((Hands - Joints[StickJoint::ChestJoint]) * FVector(1., 1., 0.)).GetSafeNormal(UE_SMALL_NUMBER, Subject->GetActorForwardVector());
	double Nearest = AimedAtWithin;
	bool bFound = false;
	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		if (It->Team == Subject->Team)
		{
			continue;
		}
		const TArray<FVector> Theirs = It->GetJointPositions();
		// Nobody aims at someone lying down. Read from the pose, which is all a puppet has to go by.
		const bool bOnItsFeet = Theirs[StickJoint::SkullJoint].Z - FMath::Min(Theirs[StickJoint::LeftFoot].Z, Theirs[StickJoint::RightFoot].Z) > 90.;
		const FVector To = (Theirs[StickJoint::PelvisJoint] - Pelvis) * FVector(1., 1., 0.);
		if (bOnItsFeet && To.Size() < Nearest && (To.GetSafeNormal() | Aim) > AimedAtCosine)
		{
			Nearest = To.Size();
			OutPoint = Theirs[StickJoint::PelvisJoint];
			bFound = true;
		}
	}
	return bFound;
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
	AimedUntilSeconds = 0.f;
}

void AFightCamera::HandleHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	Jolt.Add(IsHeavyMove(Move) ? JoltOnHeavyHit : JoltOnPunch);
}

void AFightCamera::HandleBlocked(AActor* Attacker, AActor* Defender)
{
	Jolt.Add(JoltOnBlock);
}
