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
	};
	const FPropLook Looks[] = {
		{ TEXT("SM_Crate"), 30.f, false, 0.f, 0, false },
		{ TEXT("SM_Bottle"), 22.f, true, 0.f, 0, false },
		{ TEXT("SM_Melon"), 26.f, true, 0.f, 0, false },
		{ TEXT("SM_Cone"), 18.f, false, 0.f, 0, false },
		{ TEXT("SM_Pan"), 34.f, false, 0.f, 0, false },
		{ TEXT("SM_GoldHammer"), 999.f, false, 999.f, 3, true },
		{ TEXT("SM_GoldBat"), 999.f, false, 999.f, 3, true },
		{ TEXT("SM_GoldFish"), 999.f, false, 999.f, 3, true },
		{ TEXT("SM_WoodBat"), 24.f, false, 22.f, 8, false },
		{ TEXT("SM_IronBar"), 30.f, false, 30.f, 12, false },
		{ TEXT("SM_Sledge"), 40.f, false, 45.f, 6, false },
	};

	const float LegendaryGlow = 1.6f;
	// A weapon in the hands points up and ahead, and is held near its handle end.
	const double HeldPitchDegrees = 62.;
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
}

AThrownProp::AThrownProp()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AThrownProp::IsLegendary() const
{
	return Looks[static_cast<int32>(Kind)].bLegendary;
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
	AddActorWorldOffset(FVector(0., 0., HalfHeight));
	Home = GetActorTransform();
}

void AThrownProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPuppet)
	{
		return;
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
				const FRotator Raised(HeldPitchDegrees, Carrier->GetActorRotation().Yaw, 0.);
				SetActorLocationAndRotation(Carrier->GetCarryPoint() + Raised.Vector() * (HalfLength * HeldAtShareOfLength), Raised);
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
	AddActorLocalRotation(FRotator(SpinDegreesPerSecond * DeltaSeconds, 0., 0.));

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

	if (Location.Z <= FloorHeight + HalfHeight)
	{
		Location.Z = FloorHeight + HalfHeight;
		SetActorLocationAndRotation(Location, FRotator(0., GetActorRotation().Yaw, 0.));
		if ((bCanStillHit && Looks[static_cast<int32>(Kind)].bShatters) || (bThrown && IsLegendary()))
		{
			Shatter();
		}
		else
		{
			State = EState::Resting;
		}
		bCanStillHit = false;
		return;
	}
	SetActorLocation(Location);
}

void AThrownProp::PickUp(AStickFighter* NewHolder)
{
	Holder = NewHolder;
	State = EState::Held;
	// It is placed between the carrier's hands as they are this tick, not as they were last tick.
	AddTickPrerequisiteActor(NewHolder);
}

void AThrownProp::Release(const FVector& InVelocity)
{
	const AStickFighter* Thrower = Holder.Get();
	ThrowerTeam = Thrower ? Thrower->Team : INDEX_NONE;
	Velocity = InVelocity;
	bCanStillHit = !InVelocity.IsNearlyZero();
	bThrown = bCanStillHit;
	State = EState::Flying;
}

void AThrownProp::SetPuppetTransform(const FTransform& Transform)
{
	bPuppet = true;
	SetActorTransform(Transform);
}

void AThrownProp::Shatter()
{
	State = EState::Shattered;
	AddActorWorldOffset(FVector(0., 0., -GoneDepth));
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
	bInReserve = DropOnKnockout > 0;
	UsesLeft = Looks[static_cast<int32>(Kind)].Uses;
	bCanStillHit = false;
	State = EState::Resting;
	Holder = nullptr;
	Velocity = FVector::ZeroVector;
	SetActorTransform(Home);
}

void AThrownProp::HoldInReserve(int32 LatestKnockoutToDropOn)
{
	LatestKnockout = FMath::Max(LatestKnockoutToDropOn, 1);
	DropOnKnockout = FMath::RandRange(1, LatestKnockout);
	bInReserve = true;
	AddActorWorldOffset(FVector(0., 0., -ReserveDepth));
	Home = GetActorTransform();
}

void AThrownProp::HandleFighterDown(AActor* Fighter, bool bPlayer)
{
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
