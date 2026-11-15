#include "StickToTheScript/Fighter/ThrownProp.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// What each kind of prop is. Indexed by EPropKind.
	struct FPropLook
	{
		// Its mesh in Content/Props, built by Tools/build_props.py.
		const TCHAR* Mesh;
		// What it does to whoever it is thrown at.
		float Damage;
		bool bShatters;
		// More than 0 for something held and swung: what a blow with it does, and how many blows it lasts.
		float SwingDamage;
		int32 Uses;
		// A golden prop knocks out whatever it touches and shines by its own light.
		bool bLegendary;
		// What its pieces look like when it breaks.
		FLinearColor Pieces;
	};
	const FPropLook Looks[] = {
		{ TEXT("SM_Crate"), 30.f, false, 0.f, 0, false, FLinearColor(0.55f, 0.38f, 0.2f) },
		{ TEXT("SM_Bottle"), 22.f, true, 0.f, 0, false, FLinearColor(0.1f, 0.45f, 0.2f) },
		// The melon is a bar stool now and the cone a wet-floor sign: scans of real things, under the names the scenes use.
		{ TEXT("SM_Melon"), 26.f, false, 0.f, 0, false, FLinearColor(0.35f, 0.22f, 0.12f) },
		{ TEXT("SM_Cone"), 18.f, false, 0.f, 0, false, FLinearColor(0.95f, 0.75f, 0.05f) },
		{ TEXT("SM_Pan"), 34.f, false, 0.f, 0, false, FLinearColor(0.2f, 0.2f, 0.22f) },
		{ TEXT("SM_GoldHammer"), 999.f, false, 999.f, 3, true, FLinearColor(1.f, 0.75f, 0.15f) },
		{ TEXT("SM_GoldBat"), 999.f, false, 999.f, 3, true, FLinearColor(1.f, 0.75f, 0.15f) },
		{ TEXT("SM_GoldFish"), 999.f, false, 999.f, 3, true, FLinearColor(1.f, 0.75f, 0.15f) },
		{ TEXT("SM_WoodBat"), 24.f, false, 22.f, 8, false, FLinearColor(0.5f, 0.33f, 0.18f) },
		{ TEXT("SM_IronBar"), 30.f, false, 30.f, 12, false, FLinearColor(0.3f, 0.3f, 0.33f) },
		{ TEXT("SM_Sledge"), 40.f, false, 45.f, 6, false, FLinearColor(0.5f, 0.33f, 0.18f) },
		{ TEXT("SM_Pistol"), 16.f, false, 0.f, 0, false, FLinearColor(0.08f, 0.08f, 0.09f) },
		// No scan of a shotgun has been found. Until one is, the blaster is on no set and in no shop, and borrows the rifle's shape.
		{ TEXT("SM_Rifle"), 28.f, false, 0.f, 0, false, FLinearColor(0.12f, 0.12f, 0.14f) },
		{ TEXT("SM_Rifle"), 24.f, false, 0.f, 0, false, FLinearColor(0.1f, 0.1f, 0.11f) },
	};

	// What makes a prop a gun. The punch fires it while it has rounds left; empty, it is thrown like anything else.
	struct FGunLook
	{
		EPropKind Kind;
		const TCHAR* Name;
		int32 Rounds;
		// The blaster sends several pellets with every shot, each doing this much.
		int32 Pellets;
		float PelletDamage;
		float SecondsBetweenShots;
		float SpreadDegrees;
		// How hard whoever is hit is thrown back.
		float KnockSpeed;
		EMove Move;
		// The model is kept by its middle and held by its grip: how far the middle is ahead of the grip, and above it.
		float GripToMiddle;
		float GripUnderMiddle;
	};
	const FGunLook Guns[] = {
		{ EPropKind::Pistol, TEXT("PISTOL"), 8, 1, 26.f, 0.26f, 1.2f, 420.f, EMove::Shot, 7.f, 3.5f },
		// A bolt-action rifle: one round a second, and one is enough for most.
		{ EPropKind::Rifle, TEXT("RIFLE"), 5, 1, 70.f, 0.95f, 0.4f, 760.f, EMove::Shot, 27.f, 4.f },
		{ EPropKind::Blaster, TEXT("BLASTER"), 4, 7, 13.f, 0.7f, 8.f, 950.f, EMove::Blast, 27.f, 4.f },
	};

	const FGunLook* FindGun(EPropKind Kind)
	{
		for (const FGunLook& Gun : Guns)
		{
			if (Gun.Kind == Kind)
			{
				return &Gun;
			}
		}
		return nullptr;
	}

	// A gun kicks up as it fires and comes back down over this long.
	const float GunKickSeconds = 0.16f;
	const double GunKickDegrees = 24.;
	const double GunmanDropHeight = 110.;
	// A gun longer than twice this is shouldered. Anything else smaller than twice the second is held in one hand.
	const double LongGunOverHalfLength = 30.;
	const double OneHandUnderHalfSize = 26.;
	// A long thing in one hand points up and this far back past upright, and the fist is this share of its half
	// length below its middle.
	const double OneHandPitchDegrees = 112.;
	const double OneHandHeldAtShare = 0.7;

	const float LegendaryGlow = 1.6f;
	// A weapon in the hands points up and ahead, and is held near its handle end.
	const double HeldPitchDegrees = 62.;
	const double SwungPitchDegrees = -30.;
	const double HeldAtShareOfLength = 0.55;
	const double LegendaryDropHeight = 320.;
	const float ThrownLegendLaunchSpeed = 950.f;
	const float LegendarySpinDegreesPerSecond = 140.f;
	// Where a prop in reserve waits: under the floor, out of every picture.
	const double ReserveDepth = 1500.;

	const float PropGravity = 980.f;
	const double HitRadius = 65.;
	const float SpinDegreesPerSecond = 540.f;
	// A prop may fly this far past the arena before the set's walls stop it.
	const double BeyondArena = 150.;
	// Where a shattered bottle goes. Its recorded place carries that into the film, so nothing else has to remember it broke.
	const double GoneDepth = 2000.;

	// A prop that comes down faster than this bounces, keeping this share of its speed upward and this share along the floor.
	const float BounceAboveSpeed = 240.f;
	const float BounceKeepsUp = 0.36f;
	const float BounceKeepsAlong = 0.5f;
	const float SettleSeconds = 0.14f;
	// As a take is called, everything on the set is dropped into its place from between these heights.
	const double DropInLowest = 80.;
	const double DropInHighest = 170.;
	const int32 PieceCount = 10;
}

DEFINE_LOG_CATEGORY_STATIC(LogThrownProp, Log, All);

AThrownProp::AThrownProp()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shiny(TEXT("/Game/Materials/M_Figure"));
	PieceMesh = Cube.Object;
	PieceMaterial = Shiny.Object;
}

bool AThrownProp::IsLegendary() const
{
	return Looks[static_cast<int32>(Kind)].bLegendary;
}

bool AThrownProp::IsGun() const
{
	return FindGun(Kind) != nullptr;
}

const TCHAR* AThrownProp::GetGunName() const
{
	const FGunLook* Gun = FindGun(Kind);
	return Gun ? Gun->Name : TEXT("");
}

bool AThrownProp::TakeShot(int32& OutPellets, float& OutPelletDamage, float& OutSpreadDegrees, float& OutKnockSpeed, EMove& OutMove)
{
	const FGunLook* Gun = FindGun(Kind);
	const float Now = GetWorld()->GetTimeSeconds();
	if (!Gun || RoundsLeft <= 0 || Now - LastShotSeconds < Gun->SecondsBetweenShots)
	{
		return false;
	}

	--RoundsLeft;
	LastShotSeconds = Now;
	OutPellets = Gun->Pellets;
	OutPelletDamage = Gun->PelletDamage;
	OutSpreadDegrees = Gun->SpreadDegrees;
	OutKnockSpeed = Gun->KnockSpeed;
	OutMove = Gun->Move;
	return true;
}

void AThrownProp::HoldInReserveFor(AActor* Fighter)
{
	ReservedFor = Fighter;
	bInReserve = true;
	AddActorWorldOffset(FVector(0., 0., -ReserveDepth));
	Home = GetActorTransform();
}

EPropHold AThrownProp::GetHold() const
{
	if (IsGun())
	{
		return HalfLength > LongGunOverHalfLength ? EPropHold::LongGun : EPropHold::Pistol;
	}
	if (IsSwung())
	{
		return EPropHold::Swung;
	}
	return FMath::Max(HalfLength, HalfHeight) < OneHandUnderHalfSize ? EPropHold::OneHand : EPropHold::Overhead;
}

bool AThrownProp::IsSwung() const
{
	return Looks[static_cast<int32>(Kind)].SwingDamage > 0.f;
}

float AThrownProp::GetSwingDamage() const
{
	return Looks[static_cast<int32>(Kind)].SwingDamage;
}

void AThrownProp::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UTakeEvents>()->OnTakeStarted.AddDynamic(this, &AThrownProp::HandleTakeStarted);
	GetWorld()->GetSubsystem<UTakeEvents>()->OnFighterDown.AddDynamic(this, &AThrownProp::HandleFighterDown);
}

void AThrownProp::SetUp(EPropKind InKind, double InFloorHeight, const FVector2D& InArenaMin, const FVector2D& InArenaMax)
{
	Kind = InKind;
	FloorHeight = InFloorHeight;
	ArenaMin = InArenaMin - FVector2D(BeyondArena);
	ArenaMax = InArenaMax + FVector2D(BeyondArena);

	const FPropLook& Look = Looks[static_cast<int32>(Kind)];
	// By name and not by a hard reference, so a new prop is one more row in the table.
	UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Props/%s.%s"), Look.Mesh, Look.Mesh));
	if (ensureMsgf(Shape, TEXT("The prop mesh %s is missing. Run Tools/build_props.py."), Look.Mesh))
	{
		Mesh->SetStaticMesh(Shape);
		HalfHeight = Shape->GetBounds().BoxExtent.Z;
		HalfLength = Shape->GetBounds().BoxExtent.X;
	}
	if (Look.bLegendary)
	{
		Mesh->CreateDynamicMaterialInstance(0)->SetScalarParameterValue("Glow", LegendaryGlow);
	}
	UsesLeft = Look.Uses;
	RoundsLeft = IsGun() ? FindGun(Kind)->Rounds : 0;
	AddActorWorldOffset(FVector(0., 0., HalfHeight));
	Home = GetActorTransform();
}

void AThrownProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Before anything else: the pieces of a prop that broke fly on in the film too, where the prop itself is only placed.
	FlyPieces(DeltaSeconds);
	if (bPuppet)
	{
		return;
	}

	if (State == EState::Resting && SettleSecondsLeft > 0.f)
	{
		SettleSecondsLeft = FMath::Max(SettleSecondsLeft - DeltaSeconds, 0.f);
		const FQuat Upright = FRotator(0., SettleFrom.Yaw, 0.).Quaternion();
		SetActorRotation(FQuat::Slerp(SettleFrom.Quaternion(), Upright, 1.f - SettleSecondsLeft / SettleSeconds));
	}

	// A legendary prop lying on the set turns slowly, so it cannot be mistaken for a crate.
	if (State == EState::Resting && IsLegendary() && GetActorLocation().Z > FloorHeight)
	{
		AddActorWorldRotation(FRotator(0., LegendarySpinDegreesPerSecond * DeltaSeconds, 0.));
	}

	if (State == EState::Held)
	{
		if (const AStickFighter* Carrier = Holder.Get())
		{
			if (IsSwung())
			{
				// Up and ahead while it is carried, brought over and down onto its target as a swing lands.
				const FRotator Raised(FMath::Lerp(HeldPitchDegrees, SwungPitchDegrees, static_cast<double>(Carrier->GetWeaponSwing())), Carrier->GetActorRotation().Yaw, 0.);
				SetActorLocationAndRotation(Carrier->GetCarryPoint() + Raised.Vector() * (HalfLength * HeldAtShareOfLength), Raised);
			}
			else if (const FGunLook* Gun = FindGun(Kind))
			{
				// A gun points where its carrier faces, and kicks up as it fires. It sits in the hand by its grip: the right
				// hand for one that is shouldered, between the two hands for a pistol.
				const double Kick = GunKickDegrees * FMath::Clamp(1. - (GetWorld()->GetTimeSeconds() - LastShotSeconds) / GunKickSeconds, 0., 1.);
				const FRotator Aimed(Kick, Carrier->GetActorRotation().Yaw, 0.);
				const FVector Grip = GetHold() == EPropHold::LongGun ? Carrier->GetGripPoint(1) : Carrier->GetCarryPoint();
				SetActorLocationAndRotation(Grip + Aimed.Vector() * Gun->GripToMiddle + FVector(0., 0., Gun->GripUnderMiddle), Aimed);
			}
			else if (GetHold() == EPropHold::OneHand)
			{
				if (HalfLength > HalfHeight)
				{
					// Something long, a bottle: held by its end in the right fist, standing up and a little back
					// out of it, as it is held to be thrown. Laid level on the palm it floated behind the fingers.
					const FRotator Cocked(OneHandPitchDegrees, Carrier->GetActorRotation().Yaw, 0.);
					SetActorLocationAndRotation(Carrier->GetGripPoint(1) + Cocked.Vector() * (HalfLength * OneHandHeldAtShare), Cocked);
				}
				else
				{
					// Something upright: on the palm of the right hand, by its lower end.
					SetActorLocationAndRotation(Carrier->GetGripPoint(1) + FVector(0., 0., HalfHeight * 0.55), FRotator(0., Carrier->GetActorRotation().Yaw, 0.));
				}
			}
			else
			{
				SetActorLocationAndRotation(Carrier->GetCarryPoint() + FVector(0., 0., HalfHeight), FRotator(0., Carrier->GetActorRotation().Yaw, 0.));
			}
		}
		return;
	}
	if (State != EState::Flying)
	{
		return;
	}

	Velocity.Z -= PropGravity * DeltaSeconds;
	FVector Location = GetActorLocation() + Velocity * DeltaSeconds;
	AddActorLocalRotation(FRotator(SpinDegreesPerSecond * SpinShare * DeltaSeconds, 0., 0.));

	if (bCanStillHit)
	{
		for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
		{
			AStickFighter* Fighter = *It;
			const FVector Feet = Fighter->GetActorLocation();
			const double Height = Location.Z - Feet.Z;
			// It passes over a ducked head.
			if (Fighter->Team == ThrowerTeam || Fighter->IsDown() || FVector::Dist2D(Location, Feet) > HitRadius
				|| Height < 20. || Height > (Fighter->IsDucking() ? 110. : 195.))
			{
				continue;
			}

			bCanStillHit = false;
			Fighter->SufferBlow(Holder.Get(), Looks[static_cast<int32>(Kind)].Damage, Location, Velocity, true, EMove::Throw);
			if (IsLegendary())
			{
				Fighter->Launch(Velocity, ThrownLegendLaunchSpeed);
				SpendUse();
				return;
			}
			if (Looks[static_cast<int32>(Kind)].bShatters)
			{
				Shatter();
				return;
			}
			Velocity = FVector(-Velocity.X * 0.15, -Velocity.Y * 0.15, 260.);
			break;
		}
	}

	// The set's walls stop it dead, and it drops.
	const FVector2D Held(FMath::Clamp(Location.X, ArenaMin.X, ArenaMax.X), FMath::Clamp(Location.Y, ArenaMin.Y, ArenaMax.Y));
	if (!Held.Equals(FVector2D(Location), 0.01))
	{
		Location.X = Held.X;
		Location.Y = Held.Y;
		Velocity.X = 0.;
		Velocity.Y = 0.;
	}

	// Thrown or dropped, it lands on the floor. Only the props crew puts things on a counter.
	const double Ground = bDroppingIn ? Home.GetLocation().Z : FloorHeight + HalfHeight;
	if (Location.Z <= Ground)
	{
		Location.Z = Ground;
		SetActorLocation(Location);
		const bool bBreaks = (bCanStillHit && Looks[static_cast<int32>(Kind)].bShatters) || (bThrown && IsLegendary());
		bCanStillHit = false;
		if (bBreaks)
		{
			Shatter();
		}
		else if (Velocity.Z < -BounceAboveSpeed)
		{
			// It bounces, lower each time, and a bounce hurts nobody.
			Velocity = FVector(Velocity.X * BounceKeepsAlong, Velocity.Y * BounceKeepsAlong, -Velocity.Z * BounceKeepsUp);
			SpinShare *= 0.5f;
		}
		else
		{
			SettleFrom = GetActorRotation();
			SettleSecondsLeft = SettleSeconds;
			State = EState::Resting;
		}
		return;
	}
	SetActorLocation(Location);
}

void AThrownProp::PickUp(AStickFighter* NewHolder)
{
	UE_LOG(LogThrownProp, Log, TEXT("%s picks up %s"), *GetNameSafe(NewHolder), Looks[static_cast<int32>(Kind)].Mesh);
	Holder = NewHolder;
	State = EState::Held;
	// It is placed between the carrier's hands as they are this tick, not as they were last tick.
	AddTickPrerequisiteActor(NewHolder);
}

void AThrownProp::Release(const FVector& InVelocity)
{
	const AStickFighter* Thrower = Holder.Get();
	ThrowerTeam = Thrower ? Thrower->Team : INDEX_NONE;
	bDroppingIn = false;
	Velocity = InVelocity;
	bCanStillHit = !InVelocity.IsNearlyZero();
	bThrown = bCanStillHit;
	SpinShare = 1.f;
	State = EState::Flying;
}

void AThrownProp::SetPuppetTransform(const FTransform& Transform)
{
	// A prop that broke in the take is recorded as dropping out of the world, deeper than one held in reserve ever is.
	// The film breaks it again where it was.
	const FVector Moved = Transform.GetLocation() - GetActorLocation();
	if (bPuppet && GetActorLocation().Z > FloorHeight - 100. && Transform.GetLocation().Z < FloorHeight - (ReserveDepth + GoneDepth) * 0.5)
	{
		BurstIntoPieces(GetActorLocation(), PuppetVelocity);
	}
	else if (bPuppet && GetWorld()->GetDeltaSeconds() > UE_KINDA_SMALL_NUMBER)
	{
		PuppetVelocity = Moved / GetWorld()->GetDeltaSeconds();
	}
	bPuppet = true;
	SetActorTransform(Transform);
}

void AThrownProp::Shatter()
{
	BurstIntoPieces(GetActorLocation(), Velocity);
	State = EState::Shattered;
	AddActorWorldOffset(FVector(0., 0., -GoneDepth));
}

void AThrownProp::BurstIntoPieces(const FVector& From, const FVector& Carried)
{
	while (Pieces.Num() < PieceCount)
	{
		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
		Piece->SetStaticMesh(PieceMesh);
		Piece->SetMaterial(0, PieceMaterial);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetupAttachment(RootComponent);
		// The prop itself drops out of the world when it breaks. Its pieces stay where it broke.
		Piece->SetUsingAbsoluteLocation(true);
		Piece->SetUsingAbsoluteRotation(true);
		Piece->SetUsingAbsoluteScale(true);
		Piece->RegisterComponent();
		Pieces.Add(Piece);
	}

	UE_LOG(LogThrownProp, Log, TEXT("%s breaks into pieces%s"), Looks[static_cast<int32>(Kind)].Mesh, bPuppet ? TEXT(" in the film") : TEXT(""));
	PieceVelocities.SetNum(PieceCount);
	const FLinearColor Color = Looks[static_cast<int32>(Kind)].Pieces;
	for (int32 Index = 0; Index < PieceCount; ++Index)
	{
		const float Size = FMath::FRandRange(0.035f, 0.09f);
		UStaticMeshComponent* Piece = Pieces[Index];
		Piece->SetWorldLocationAndRotation(From + FMath::VRand() * FMath::Max(HalfHeight, 4.) * 0.6, FMath::VRand().Rotation());
		Piece->SetWorldScale3D(FVector(Size, Size * FMath::FRandRange(0.4f, 0.9f), Size * FMath::FRandRange(0.2f, 0.5f)));
		// A melon is red inside its green rind, so every third piece of anything is a shade darker.
		Piece->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Index % 3 == 0 ? Color * 0.45f : Color);
		Piece->SetVisibility(true);
		PieceVelocities[Index] = Carried * 0.25 + FMath::VRand() * FMath::FRandRange(90.f, 280.f) + FVector(0., 0., FMath::FRandRange(120.f, 300.f));
	}
	bPiecesFlying = true;
}

void AThrownProp::FlyPieces(float DeltaSeconds)
{
	if (!bPiecesFlying)
	{
		return;
	}

	bPiecesFlying = false;
	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		FVector& Speed = PieceVelocities[Index];
		if (Speed.IsZero())
		{
			continue;
		}

		Speed.Z -= PropGravity * DeltaSeconds;
		FVector Location = Pieces[Index]->GetComponentLocation() + Speed * DeltaSeconds;
		if (Location.Z <= FloorHeight + 1.5)
		{
			// It lies where it lands, flat.
			Location.Z = FloorHeight + 1.5;
			Speed = FVector::ZeroVector;
			Pieces[Index]->SetWorldRotation(FRotator(0., Pieces[Index]->GetComponentRotation().Yaw, 0.));
		}
		else
		{
			Pieces[Index]->AddWorldRotation(FRotator(500. * DeltaSeconds, 300. * DeltaSeconds, 0.));
			bPiecesFlying = true;
		}
		Pieces[Index]->SetWorldLocation(Location);
	}
}

void AThrownProp::HandleTakeStarted(int32 TakeNumber)
{
	bPuppet = false;
	KnockoutsSeen = 0;
	bThrown = false;
	if (LatestKnockout > 0)
	{
		DropOnKnockout = FMath::RandRange(1, LatestKnockout);
	}
	bInReserve = DropOnKnockout > 0 || ReservedFor.IsValid();
	UsesLeft = Looks[static_cast<int32>(Kind)].Uses;
	RoundsLeft = IsGun() ? FindGun(Kind)->Rounds : 0;
	LastShotSeconds = -10.f;
	bCanStillHit = false;
	State = EState::Resting;
	Holder = nullptr;
	Velocity = FVector::ZeroVector;
	SettleSecondsLeft = 0.f;
	SetActorTransform(Home);
	bPiecesFlying = false;
	for (UStaticMeshComponent* Piece : Pieces)
	{
		Piece->SetVisibility(false);
	}

	// The props crew drops everything back in its place as the take is called.
	if (!bInReserve && !bOffTheSet)
	{
		AddActorWorldOffset(FVector(0., 0., FMath::FRandRange(DropInLowest, DropInHighest)));
		Release(FVector::ZeroVector);
		bDroppingIn = true;
	}
}

void AThrownProp::HoldInReserve(int32 LatestKnockoutToDropOn)
{
	LatestKnockout = FMath::Max(LatestKnockoutToDropOn, 1);
	DropOnKnockout = FMath::RandRange(1, LatestKnockout);
	bInReserve = true;
	AddActorWorldOffset(FVector(0., 0., -ReserveDepth));
	Home = GetActorTransform();
}

bool AThrownProp::FallFromHandOf(AActor* Fighter)
{
	if (Fighter != ReservedFor.Get() || !bInReserve)
	{
		return false;
	}
	const FVector Fallen = Fighter->GetActorLocation();
	SetActorLocation(FVector(FMath::Clamp(Fallen.X, ArenaMin.X, ArenaMax.X), FMath::Clamp(Fallen.Y, ArenaMin.Y, ArenaMax.Y), FloorHeight + GunmanDropHeight));
	bInReserve = false;
	Holder = nullptr;
	Release(FVector::ZeroVector);
	UE_LOG(LogThrownProp, Log, TEXT("%s falls from the hand of %s at %.0f, %.0f"), Looks[static_cast<int32>(Kind)].Mesh, *GetNameSafe(Fighter), GetActorLocation().X, GetActorLocation().Y);
	return true;
}

void AThrownProp::HandleFighterDown(AActor* Fighter, bool bPlayer)
{
	if (ReservedFor.IsValid())
	{
		// A gunman's pistol falls from his hand where he goes down.
		FallFromHandOf(Fighter);
		return;
	}

	if (bPlayer || DropOnKnockout == 0 || !Fighter || ++KnockoutsSeen != DropOnKnockout)
	{
		return;
	}

	// It falls out of the air over the fallen opponent, and lands like anything else that is let go.
	const FVector Fallen = Fighter->GetActorLocation();
	SetActorLocation(FVector(FMath::Clamp(Fallen.X, ArenaMin.X, ArenaMax.X), FMath::Clamp(Fallen.Y, ArenaMin.Y, ArenaMax.Y), FloorHeight + LegendaryDropHeight));
	bInReserve = false;
	Holder = nullptr;
	Release(FVector::ZeroVector);
}

bool AThrownProp::SpendUse()
{
	if (--UsesLeft > 0)
	{
		return false;
	}
	Shatter();
	return true;
}

void AThrownProp::KeepOffTheSet()
{
	bOffTheSet = true;
	AddActorWorldOffset(FVector(0., 0., -ReserveDepth));
	Home = GetActorTransform();
}

void AThrownProp::BringOnSet(EPropKind InKind, const FVector2D& Where)
{
	bOffTheSet = false;
	SetActorLocationAndRotation(FVector(Where.X, Where.Y, FloorHeight), FRotator::ZeroRotator);
	// SetUp widens the arena it is given, and this prop's has been widened once already.
	SetUp(InKind, FloorHeight, ArenaMin + FVector2D(BeyondArena), ArenaMax - FVector2D(BeyondArena));
}
