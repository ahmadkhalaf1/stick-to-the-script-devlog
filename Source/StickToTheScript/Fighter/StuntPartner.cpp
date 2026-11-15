#include "StickToTheScript/Fighter/StuntPartner.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/ThrownProp.h"

namespace
{
	// How each kind of opponent fights freely. Indexed by EOpponentKind.
	struct FStyle
	{
		float Health;
		float Speed;
		// How far from the player it stands to strike.
		float Range;
		// How long its strike takes to arrive, which is the player's time to answer it.
		float WindUpSeconds;
		float RestSeconds;
		float KickShare;
		float GuardShare;
		float PunchDamage;
		float KickDamage;
		bool bArmored;
		// Steps away after every strike instead of staying in the player's face.
		bool bBacksOff;
		float Thickness;
		float HeadRadius;
		// Now and then its punch or its kick is one of the bigger moves instead. Each kind has its own, so they can be told apart.
		EMove PunchSpecial;
		EMove KickSpecial;
		float SpecialShare;
	};
	const FStyle Styles[] = {
		// Their health is a fifth more than it was until Sun Nov 15: with quicker keys and the big finish, the game's own
		// player was through a scene in a quarter of a minute.
		{ 65.f, 250.f, 95.f, 0.42f, 1.f, 0.25f, 0.2f, 8.f, 13.f, false, false, 7.f, 11.f, EMove::Chop, EMove::Kick, 0.2f },
		{ 42.f, 370.f, 90.f, 0.28f, 0.7f, 0.1f, 0.f, 6.f, 10.f, false, true, 5.5f, 10.f, EMove::Punch, EMove::Sweep, 0.5f },
		{ 54.f, 270.f, 100.f, 0.5f, 1.2f, 0.9f, 0.3f, 8.f, 14.f, false, true, 6.5f, 10.5f, EMove::Chop, EMove::Roundhouse, 0.4f },
		{ 200.f, 180.f, 100.f, 0.6f, 1.3f, 0.5f, 0.35f, 14.f, 22.f, true, false, 10.f, 13.5f, EMove::Uppercut, EMove::Kick, 0.25f },
		// The gunman: its range is the gap it keeps, its wind-up the time it aims, and its punch damage what a shot does.
		{ 42.f, 230.f, 450.f, 1.f, 2.6f, 0.f, 0.f, 14.f, 0.f, false, true, 6.f, 10.5f, EMove::Punch, EMove::Kick, 0.f },
		// The boss: a heavy who does not plod. He guards more, rests less, and half of what he throws is a big move.
		{ 350.f, 230.f, 100.f, 0.5f, 0.8f, 0.5f, 0.5f, 16.f, 22.f, true, false, 10.5f, 14.f, EMove::Uppercut, EMove::Roundhouse, 0.45f },
	};
	// A bigger move is shown for longer before it lands, so it can be read and answered: step back, or duck a roundhouse.
	const float SpecialWindUpScale = 1.35f;

	// No more than this many opponents come at the player at once. The others keep their distance and wait for a turn.
	const int32 MaxAttackers = 2;
	const double WaitingDistance = 240.;
	const float GuardInsideDistance = 150.f;
	const float GuardSeconds = 0.5f;

	// The aim line follows the player until this long before the shot, then holds still: the moment to get out of it.
	const float AimHoldSeconds = 0.3f;
	const double ChestHeight = 120.;
	const double BulletRadius = 32.;
	const double AimLineLength = 1600.;
	const float TracerSeconds = 0.08f;
	const double TooCloseToShoot = 180.;
	const double ShootingReach = 900.;
	const FLinearColor AimColor(0.9f, 0.02f, 0.02f);
	const FLinearColor TracerColor(1.f, 0.85f, 0.3f);
}

AStuntPartner::AStuntPartner()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));

	AimLine = CreateDefaultSubobject<UStaticMeshComponent>("AimLine");
	AimLine->SetupAttachment(RootComponent);
	AimLine->SetUsingAbsoluteLocation(true);
	AimLine->SetUsingAbsoluteRotation(true);
	AimLine->SetUsingAbsoluteScale(true);
	AimLine->SetStaticMesh(Cylinder.Object);
	AimLine->SetMaterial(0, Material.Object);
	AimLine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AimLine->SetCastShadow(false);
	AimLine->SetVisibility(false);
}

void AStuntPartner::BeginPlay()
{
	Super::BeginPlay();

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnTakeStarted.AddDynamic(this, &AStuntPartner::HandleNewTake);
	Events->OnAction.AddDynamic(this, &AStuntPartner::HandleAction);
	Events->OnCut.AddDynamic(this, &AStuntPartner::HandleCut);
	Events->OnTakeFinished.AddDynamic(this, &AStuntPartner::HandleTakeFinished);
}

void AStuntPartner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// A broken arm holds nothing: a gunman whose gun arm breaks lets his pistol fall, and fights on like anyone else.
	if (bShoots && !IsDown() && (GetBrokenArms() & 2) != 0)
	{
		bShoots = false;
		bArmed = false;
		bAiming = false;
		for (TActorIterator<AThrownProp> It(GetWorld()); It; ++It)
		{
			if (It->FallFromHandOf(this))
			{
				break;
			}
		}
	}
	if (bThinks && bFightLive && !IsDown() && PlayerFighter)
	{
		Think(DeltaSeconds);
	}
	if (bShoots)
	{
		ShowLine(DeltaSeconds);
	}
}

void AStuntPartner::ThinkWithAGun(float DeltaSeconds, const FVector& Mine, const FVector& Theirs)
{
	FaceTowards(Theirs);

	if (AimSecondsLeft > 0.f)
	{
		Walk(FVector2D::ZeroVector);
		AimSecondsLeft -= DeltaSeconds;
		if (AimSecondsLeft > AimHoldSeconds)
		{
			AimPoint = Theirs + FVector(0., 0., ChestHeight);
		}
		bAiming = true;
		if (AimSecondsLeft <= 0.f)
		{
			Fire();
		}
		return;
	}

	bAiming = false;
	const double Distance = FVector::Dist2D(Mine, Theirs);
	if (RestSecondsLeft <= 0.f && Distance > TooCloseToShoot && Distance < ShootingReach && CanAct())
	{
		AimSecondsLeft = WindUpSeconds;
		AimPoint = Theirs + FVector(0., 0., ChestHeight);
		return;
	}

	// It keeps a gap: it backs away from a player who comes close and follows one who runs.
	const double Side = Mine.X >= Theirs.X ? 1. : -1.;
	const FVector2D Wanted(Theirs.X + Side * PreferredRange, Theirs.Y + ((PartnerIndex % 3) - 1) * 75.);
	const FVector2D Step = ClampToArena(Wanted) - FVector2D(Mine);
	const double StepLength = Step.Size();
	if (StepLength < 30.)
	{
		bSettled = true;
	}
	else if (StepLength > 80.)
	{
		bSettled = false;
	}
	Walk(bSettled ? FVector2D::ZeroVector : Step / StepLength);
}

bool AStuntPartner::IsAboutToFire() const
{
	return bAiming && !IsDown() && AimSecondsLeft <= AimHoldSeconds + 0.1f;
}

void AStuntPartner::Fire()
{
	const FVector Muzzle = GetJointPositions()[StickJoint::RightHand];
	const FVector Direction = (AimPoint - Muzzle).GetSafeNormal();
	const FVector Chest = PlayerFighter->GetActorLocation() + FVector(0., 0., ChestHeight);

	// The shot goes where the line was held. A player who left the line, ducked under it or jumped over it is missed.
	const double Along = (Chest - Muzzle) | Direction;
	const bool bHits = Along > 0. && FVector::Dist(Chest, Muzzle + Direction * Along) < BulletRadius
		&& !PlayerFighter->IsDucking() && !PlayerFighter->IsDown();

	TracerEnd = bHits ? Chest : Muzzle + Direction * AimLineLength;
	TracerSecondsLeft = TracerSeconds;
	RestSecondsLeft = RestSeconds * FMath::FRandRange(0.8f, 1.3f);

	GetWorld()->GetSubsystem<UTakeEvents>()->OnSwing.Broadcast(this, EMove::Shot);
	if (bHits)
	{
		PlayerFighter->SufferBlow(this, PunchDamage, Chest, Direction, true, EMove::Shot);
	}
}

void AStuntPartner::ShowLine(float DeltaSeconds)
{
	TracerSecondsLeft -= DeltaSeconds;
	const bool bTracer = TracerSecondsLeft > 0.f;
	const bool bShown = bTracer || (bAiming && !IsDown());
	AimLine->SetVisibility(bShown);
	if (!bShown)
	{
		return;
	}

	const FVector Muzzle = GetJointPositions()[StickJoint::RightHand];
	const FVector End = bTracer ? TracerEnd : Muzzle + (AimPoint - Muzzle).GetSafeNormal() * AimLineLength;
	// Thin while it follows the player, thicker once it holds, and a bright streak for the shot itself.
	const double LineThickness = bTracer ? 4. : AimSecondsLeft > AimHoldSeconds ? 1. : 2.4;
	AimLine->SetWorldLocationAndRotation((Muzzle + End) * 0.5, FRotationMatrix::MakeFromZ(End - Muzzle).Rotator());
	AimLine->SetWorldScale3D(FVector(LineThickness, LineThickness, FVector::Dist(Muzzle, End)) / 100.);
	AimLine->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", bTracer ? TracerColor : AimColor);
}

void AStuntPartner::StartThinking(EOpponentKind Kind, float InWakeDistance)
{
	const FStyle& Style = Styles[FMath::Clamp(static_cast<int32>(Kind), 0, static_cast<int32>(UE_ARRAY_COUNT(Styles)) - 1)];
	MaxHealth = Style.Health;
	WalkSpeed = Style.Speed;
	PunchDamage = Style.PunchDamage;
	KickDamage = Style.KickDamage;
	bArmored = Style.bArmored;
	Thickness = Style.Thickness;
	HeadRadius = Style.HeadRadius;
	// In the order of EOpponentKind.
	const TCHAR* const Roles[] = { TEXT("Thug"), TEXT("Rusher"), TEXT("Kicker"), TEXT("Heavy"), TEXT("Gunman"), TEXT("Boss") };
	WearCast(Roles[FMath::Clamp(static_cast<int32>(Kind), 0, static_cast<int32>(UE_ARRAY_COUNT(Roles)) - 1)]);

	PreferredRange = Style.Range;
	WindUpSeconds = Style.WindUpSeconds;
	RestSeconds = Style.RestSeconds;
	KickShare = Style.KickShare;
	GuardShare = Style.GuardShare;
	bBacksOff = Style.bBacksOff;
	PunchSpecial = Style.PunchSpecial;
	KickSpecial = Style.KickSpecial;
	SpecialShare = Style.SpecialShare;
	bShoots = Kind == EOpponentKind::Gunman;
	bArmed = bShoots;
	bBoss = Kind == EOpponentKind::Boss;

	WakeDistance = InWakeDistance;
	bThinks = true;
}

void AStuntPartner::Think(float DeltaSeconds)
{
	RestSecondsLeft -= DeltaSeconds;
	GuardSecondsLeft -= DeltaSeconds;

	if (PlayerFighter->IsDown())
	{
		Walk(FVector2D::ZeroVector);
		AimSecondsLeft = 0.f;
		bAiming = false;
		return;
	}

	const FVector Mine = GetActorLocation();
	const FVector Theirs = PlayerFighter->GetActorLocation();

	if (Brain == EBrain::Asleep)
	{
		// A blow wakes anyone.
		if (!ShouldWake() && GetHealthFraction() >= 1.f)
		{
			return;
		}
		Brain = EBrain::Entering;
		if (bBoss)
		{
			// The hero has fought the whole scene to get here. Make-up puts him right, and the last fight starts even.
			PlayerFighter->CatchBreath();
		}
		MoveTo(SpotLocation, static_cast<float>(FVector::Dist2D(Mine, SpotLocation)) / WalkSpeed, Theirs);
		return;
	}
	if (Brain == EBrain::Entering)
	{
		if (IsTravelling())
		{
			return;
		}
		Brain = EBrain::Waiting;
	}

	if (IsStunned())
	{
		// Being hit costs it its turn, so another opponent can step in.
		bHasTurn = false;
		Brain = EBrain::Waiting;
		RestSecondsLeft = FMath::Max(RestSecondsLeft, 0.4f);
		AimSecondsLeft = 0.f;
		bAiming = false;
		Walk(FVector2D::ZeroVector);
		return;
	}

	if (bShoots)
	{
		ThinkWithAGun(DeltaSeconds, Mine, Theirs);
		return;
	}

	if (Brain == EBrain::Striking)
	{
		// It commits to where the player was when it started: stepping out of the way is a real answer.
		if (IsStriking())
		{
			return;
		}
		bHasTurn = false;
		Brain = EBrain::Waiting;
		RestSecondsLeft = RestSeconds * FMath::FRandRange(0.7f, 1.3f);
	}

	FaceTowards(Theirs);

	// Some opponents raise a guard when they see a strike coming at them.
	const double Distance = FVector::Dist2D(Mine, Theirs);
	const bool bPlayerStriking = PlayerFighter->IsStriking();
	if (bPlayerStriking && !bSawPlayerStrike && Distance < GuardInsideDistance && FMath::FRand() < GuardShare)
	{
		GuardSecondsLeft = GuardSeconds;
	}
	bSawPlayerStrike = bPlayerStriking;
	if (GuardSecondsLeft > 0.f)
	{
		Guard(EMove::Block);
		Walk(FVector2D::ZeroVector);
		return;
	}

	if (Brain == EBrain::Waiting && RestSecondsLeft <= 0.f && CountAttackers() < MaxAttackers)
	{
		bHasTurn = true;
		Brain = EBrain::Closing;
		bSettled = false;
	}

	FVector2D Wanted;
	double Near;
	if (Brain == EBrain::Closing)
	{
		// It comes in from the side of the player it is already on.
		const double Side = Mine.X >= Theirs.X ? 1. : -1.;
		Wanted = FVector2D(Theirs.X + Side * PreferredRange, Theirs.Y);
		Near = 14.;
	}
	else
	{
		// While waiting, even-numbered opponents hold one side of the player and odd-numbered ones the other,
		// each in its own lane, so the player ends up surrounded instead of facing a queue.
		const double Side = PartnerIndex % 2 == 0 ? 1. : -1.;
		const double Lane = ((PartnerIndex % 3) - 1) * 75.;
		const double Sway = FMath::Sin(GetWorld()->GetTimeSeconds() * 0.9 + PartnerIndex * 1.7) * 35.;
		const double Away = bBacksOff || RestSecondsLeft <= 0.f ? WaitingDistance : PreferredRange + 45.;
		Wanted = FVector2D(Theirs.X + Side * (Away + Sway), Theirs.Y + Lane);
		Near = 30.;
	}

	const FVector2D Step = ClampToArena(Wanted) - FVector2D(Mine);
	const double StepLength = Step.Size();
	if (StepLength < Near)
	{
		bSettled = true;
	}
	else if (StepLength > Near + 30.)
	{
		bSettled = false;
	}
	Walk(bSettled ? FVector2D::ZeroVector : Step / StepLength * (Brain == EBrain::Closing ? 1. : 0.75));

	if (Brain == EBrain::Closing && Distance < PreferredRange + 18. && FMath::Abs(Mine.Y - Theirs.Y) < 40. && CanAct())
	{
		Walk(FVector2D::ZeroVector);
		const bool bKicks = FMath::FRand() < KickShare;
		const bool bSpecial = FMath::FRand() < SpecialShare;
		const EMove Plain = bKicks ? EMove::Kick : EMove::Punch;
		const EMove Move = bSpecial ? (bKicks ? KickSpecial : PunchSpecial) : Plain;
		if (StrikeWith(Move, WindUpSeconds * (Move == Plain ? 1.f : SpecialWindUpScale)))
		{
			Brain = EBrain::Striking;
		}
	}
}

// Opponents join in the order the scene lists them: when the player comes near, or when the fight is running thin.
bool AStuntPartner::ShouldWake() const
{
	int32 Fighting = 0;
	for (TActorIterator<AStuntPartner> It(GetWorld()); It; ++It)
	{
		if (It->IsDown())
		{
			continue;
		}
		if (It->Brain != EBrain::Asleep)
		{
			++Fighting;
		}
		else if (It->PartnerIndex < PartnerIndex)
		{
			return false;
		}
	}
	if (bBoss)
	{
		// The boss comes on alone, when everybody else is down.
		return Fighting == 0;
	}
	return Fighting < MaxAttackers || FVector::Dist2D(PlayerFighter->GetActorLocation(), SpotLocation) < WakeDistance;
}

int32 AStuntPartner::CountAttackers() const
{
	int32 Count = 0;
	for (TActorIterator<AStuntPartner> It(GetWorld()); It; ++It)
	{
		if (It->bHasTurn && !It->IsDown())
		{
			++Count;
		}
	}
	return Count;
}

void AStuntPartner::HandleNewTake(int32 TakeNumber)
{
	bFightLive = false;
	Brain = EBrain::Asleep;
	bHasTurn = false;
	bSettled = false;
	bSawPlayerStrike = false;
	GuardSecondsLeft = 0.f;
	AimSecondsLeft = 0.f;
	TracerSecondsLeft = 0.f;
	bAiming = false;
	// A little apart, so opponents who join together do not strike together.
	RestSecondsLeft = FMath::FRandRange(0.f, 0.6f);
}

void AStuntPartner::HandleAction()
{
	bFightLive = true;
}

void AStuntPartner::HandleCut(ECutReason Reason)
{
	bFightLive = false;
	bAiming = false;
	Walk(FVector2D::ZeroVector);
}

void AStuntPartner::HandleTakeFinished(const FTake& Take)
{
	bFightLive = false;
	Walk(FVector2D::ZeroVector);
}

