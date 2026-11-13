#include "StickToTheScript/Cam/BodyCam.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"

namespace
{
	const float FieldOfView = 105.f;
	// At eye level, just clear of the head: from the chest the wearer's own guard is above the frame.
	const float AheadOfSkull = 14.f;
	const float AboveSkull = 10.f;

	// The camera keeps looking at where the opponent stands, an arm's length ahead, so a lunging head does not lose them.
	const float FocusAhead = 100.f;
	const float FocusHeight = 150.f;

	// A little of the torso's twist makes the camera feel worn. Much more and a punch swings the opponent out of frame.
	const float TorsoFollow = 0.15f;

	const float PositionLag = 18.f;
	const float RotationLag = 10.f;

	const float RestingShakeDegrees = 0.35f;
	const float JoltShakeDegrees = 7.f;
	const float JoltWhenHit = 1.f;
	const float JoltWhenHitting = 0.45f;
}

ABodyCam::ABodyCam()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("Mount");
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(FieldOfView);
	Camera->bConstrainAspectRatio = false;
}

void ABodyCam::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UTakeEvents>()->OnHit.AddDynamic(this, &ABodyCam::HandleHit);
}

void ABodyCam::Wear(AStickFighter* Fighter)
{
	Wearer = Fighter;
	bJustPutOn = true;
	// The camera reads the fighter's joints, so it must move after the fighter has.
	AddTickPrerequisiteActor(Fighter);
}

void ABodyCam::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Wearer)
	{
		return;
	}

	using namespace StickJoint;
	const TArray<FVector> Joints = Wearer->GetJointPositions();
	const FVector Right = (Joints[RightShoulder] - Joints[LeftShoulder]).GetSafeNormal();
	const FVector Up = (Joints[NeckJoint] - Joints[PelvisJoint]).GetSafeNormal();
	const FQuat TorsoFacing = FRotationMatrix::MakeFromYZ(Right, Up).ToQuat();

	const FVector Mount = Joints[SkullJoint] + Wearer->GetActorForwardVector() * AheadOfSkull + Up * AboveSkull;
	const FVector Focus = Wearer->GetActorLocation() + Wearer->GetActorForwardVector() * FocusAhead + FVector(0.f, 0.f, FocusHeight);
	const FQuat Facing = FQuat::Slerp((Focus - Mount).ToOrientationQuat(), TorsoFacing, TorsoFollow);

	if (bJustPutOn)
	{
		SetActorLocationAndRotation(Mount, Facing);
		bJustPutOn = false;
	}
	else
	{
		SetActorLocationAndRotation(
			FMath::VInterpTo(GetActorLocation(), Mount, DeltaSeconds, PositionLag),
			FMath::QInterpTo(GetActorQuat(), Facing, DeltaSeconds, RotationLag));
	}

	Camera->SetRelativeRotation(Jolt.Advance(DeltaSeconds, GetWorld()->GetTimeSeconds(), RestingShakeDegrees, JoltShakeDegrees));
}

void ABodyCam::HandleHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	if (Victim == Wearer)
	{
		Jolt.Add(JoltWhenHit);
	}
	else if (Attacker == Wearer)
	{
		Jolt.Add(JoltWhenHitting);
	}
}
