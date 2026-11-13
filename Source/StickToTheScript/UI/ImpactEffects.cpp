#include "StickToTheScript/UI/ImpactEffects.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StickToTheScript/UI/ImpactSynth.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const int32 PoolSize = 260;
	const FLinearColor BloodRed(0.22f, 0.f, 0.f);
	const FLinearColor SparkYellow(1.f, 0.8f, 0.25f);

	const int32 DropsOnPunch = 12;
	const int32 DropsOnHeavyHit = 26;
	const int32 DropsOnKnockout = 34;
	const int32 SparksOnBlock = 9;

	const float FreezeDilation = 0.05f;
	const float FreezeOnPunchSeconds = 0.05f;
	const float FreezeOnHeavyHitSeconds = 0.14f;
	const float FreezeOnBlockSeconds = 0.04f;
	const float KnockoutDilation = 0.2f;
	const float KnockoutSlowSeconds = 0.9f;

	const int32 MostTakesOfASound = 6;
	const float PitchSpreadOfABlow = 0.05f;
	// The director's voice, which has no version made in code: without a recording these are silent.
	const FName ActionCall("Action");
	const FName CutCall("Cut");
	const FName WrapCall("Wrap");

	FName NameOfSound(EImpactSound Sound)
	{
		return FName(*StaticEnum<EImpactSound>()->GetNameStringByValue(static_cast<int64>(Sound)));
	}
}

DEFINE_LOG_CATEGORY_STATIC(LogImpactSound, Log, All);

AImpactEffects::AImpactEffects()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
	Synth = CreateDefaultSubobject<UImpactSynth>("Synth");
	Synth->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	DropletMesh = Sphere.Object;
	DropletMaterial = Material.Object;
}

void AImpactEffects::BeginPlay()
{
	Super::BeginPlay();

	Droplets.SetNum(PoolSize);
	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
		Mesh->SetStaticMesh(DropletMesh);
		Mesh->SetMaterial(0, DropletMaterial);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetUsingAbsoluteLocation(true);
		Mesh->SetUsingAbsoluteRotation(true);
		Mesh->SetUsingAbsoluteScale(true);
		Mesh->RegisterComponent();
		Mesh->SetVisibility(false);
		DropletMeshes.Add(Mesh);
	}

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnHit.AddDynamic(this, &AImpactEffects::HandleHit);
	Events->OnBlocked.AddDynamic(this, &AImpactEffects::HandleBlocked);
	Events->OnSwing.AddDynamic(this, &AImpactEffects::HandleSwing);
	Events->OnBarGrabbed.AddDynamic(this, &AImpactEffects::HandleBarGrabbed);
	Events->OnTakeStarted.AddDynamic(this, &AImpactEffects::HandleTakeStarted);
	Events->OnCut.AddDynamic(this, &AImpactEffects::HandleCut);
	Events->OnFinalCutStarted.AddDynamic(this, &AImpactEffects::HandleFinalCutStarted);
	Events->OnAction.AddDynamic(this, &AImpactEffects::HandleAction);
	Events->OnTakeFinished.AddDynamic(this, &AImpactEffects::HandleTakeFinished);

	for (int32 Sound = 0; Sound < static_cast<int32>(EImpactSound::Count); ++Sound)
	{
		FindRecordings(NameOfSound(static_cast<EImpactSound>(Sound)));
	}
	for (const FName Call : { ActionCall, CutCall, WrapCall })
	{
		FindRecordings(Call);
	}
	UE_LOG(LogImpactSound, Log, TEXT("%d recorded takes found in Content/Audio. Every other sound is made in code."), Recordings.Num());

	Synth->Start();
}

void AImpactEffects::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (SlowRealSecondsLeft > 0.f)
	{
		// Counted in real time: the game's own clock is the thing being slowed.
		SlowRealSecondsLeft -= FApp::GetDeltaTime();
		if (SlowRealSecondsLeft <= 0.f)
		{
			UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		}
	}

	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		FDroplet& Drop = Droplets[Index];
		if (!Drop.bFlying)
		{
			continue;
		}

		UStaticMeshComponent* Mesh = DropletMeshes[Index];
		Drop.Velocity.Z -= 980.f * DeltaSeconds;
		Drop.SecondsLeft -= DeltaSeconds;
		FVector Location = Mesh->GetComponentLocation() + Drop.Velocity * DeltaSeconds;

		const bool bLanded = Location.Z <= Drop.FloorHeight;
		if (bLanded || Drop.SecondsLeft <= 0.f)
		{
			Drop.bFlying = false;
			if (bLanded && Drop.bLeavesMark)
			{
				Location.Z = Drop.FloorHeight + 0.3f;
				Mesh->SetWorldScale3D(FVector(Drop.Size * 3.f, Drop.Size * 3.f, 0.004f));
			}
			else
			{
				Mesh->SetVisibility(false);
			}
		}
		Mesh->SetWorldLocation(Location);
	}
}

void AImpactEffects::HandleHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	const AStickFighter* Struck = Cast<AStickFighter>(Victim);
	if (!Struck || !Attacker)
	{
		return;
	}

	const bool bHeavy = IsHeavyMove(Move);
	if (Move == EMove::Shot && bFilm)
	{
		Play(EImpactSound::Gunshot);
	}
	const bool bKnockedOut = Struck->IsDown();
	const FVector Direction = (Victim->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D();
	const int32 Drops = bKnockedOut ? DropsOnKnockout : bHeavy ? DropsOnHeavyHit : DropsOnPunch;
	Spray(Struck->GetLastImpactPoint(), Direction, Drops, BloodRed, true, Victim->GetActorLocation().Z);

	if (bKnockedOut)
	{
		SlowTime(KnockoutDilation, KnockoutSlowSeconds);
		Play(EImpactSound::Knockout);
	}
	else
	{
		SlowTime(FreezeDilation, bHeavy ? FreezeOnHeavyHitSeconds : FreezeOnPunchSeconds);
		Play(bHeavy ? EImpactSound::HeavyHit : EImpactSound::Punch);
		// The crack is for an arm that broke with this blow, which a heavy blow no longer always does.
		const uint8 Broken = Struck->GetBrokenArms() & 3;
		uint8& Heard = BrokenArmsHeard.FindOrAdd(Victim);
		if (Broken & ~Heard)
		{
			Play(EImpactSound::BoneBreak);
		}
		Heard = Broken;
	}
}

void AImpactEffects::HandleBlocked(AActor* Attacker, AActor* Defender)
{
	const AStickFighter* Guard = Cast<AStickFighter>(Defender);
	if (!Guard || !Attacker)
	{
		return;
	}

	const FVector BackAtAttacker = (Attacker->GetActorLocation() - Defender->GetActorLocation()).GetSafeNormal2D();
	Spray(Guard->GetLastImpactPoint(), BackAtAttacker, SparksOnBlock, SparkYellow, false, Defender->GetActorLocation().Z);
	SlowTime(FreezeDilation, FreezeOnBlockSeconds);
	Play(EImpactSound::Block);
}

void AImpactEffects::HandleBarGrabbed(AActor* Fighter)
{
	Play(EImpactSound::Clang);
}

void AImpactEffects::HandleSwing(AActor* Attacker, EMove Move)
{
	if (Move == EMove::Shot)
	{
		Play(EImpactSound::Gunshot);
		return;
	}

	const bool bSpins = Move == EMove::Roundhouse || Move == EMove::Sweep || Move == EMove::SwingKick;
	Play(bSpins ? EImpactSound::SpinWhoosh : EImpactSound::Whoosh);
	// The big ones are shouted, as in every dojo.
	if (Move == EMove::Roundhouse || Move == EMove::Chop || Move == EMove::Uppercut || Move == EMove::SwingKick)
	{
		Play(EImpactSound::Shout, 0.7f);
	}
}

void AImpactEffects::HandleTakeStarted(int32 TakeNumber)
{
	bFilm = false;
	BrokenArmsHeard.Reset();
	Play(EImpactSound::Clapper);
}

void AImpactEffects::HandleCut(ECutReason Reason)
{
	Play(EImpactSound::CutSting);
	PlayRecording(CutCall, 1.f, 0.f, false);
}

void AImpactEffects::HandleAction()
{
	PlayRecording(ActionCall, 1.f, 0.f, false);
}

void AImpactEffects::HandleTakeFinished(const FTake& Take)
{
	PlayRecording(WrapCall, 1.f, 0.f, false);
}

USoundBase* AImpactEffects::FindRecording(const FString& Name)
{
	// Quietly: most sounds have no recording, and that is not a fault to report for each of them.
	USoundBase* Found = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Audio/%s.%s"), *Name, *Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UE_CLOG(Found, LogImpactSound, Verbose, TEXT("Found the recording %s"), *Name);
	return Found;
}

void AImpactEffects::FindRecordings(FName Name)
{
	const int32 First = Recordings.Num();
	for (int32 Take = 1; Take <= MostTakesOfASound; ++Take)
	{
		USoundBase* Found = FindRecording(FString::Printf(TEXT("S_%s_%d"), *Name.ToString(), Take));
		if (!Found)
		{
			break;
		}
		Recordings.Add(Found);
	}
	RecordingsByName.Add(Name, FIntPoint(First, Recordings.Num() - First));
}

void AImpactEffects::Play(EImpactSound Sound, float Volume)
{
	// A knockout is shown in slow motion and its sound slows with it. Every other recording keeps its speed, or the
	// frozen frame that a blow starts with would drag the start of its own sound.
	if (!PlayRecording(NameOfSound(Sound), Volume, PitchSpreadOfABlow, Sound == EImpactSound::Knockout))
	{
		Synth->Trigger(Sound, Volume);
	}
}

bool AImpactEffects::PlayRecording(FName Name, float Volume, float PitchSpread, bool bSlowsWithTheGame)
{
	const FIntPoint* Takes = RecordingsByName.Find(Name);
	if (!Takes || Takes->Y == 0)
	{
		return false;
	}

	USoundBase* Take = Recordings[Takes->X + FMath::RandHelper(Takes->Y)];
	UE_LOG(LogImpactSound, Verbose, TEXT("Plays the recording %s"), *Take->GetName());
	// The engine leaves a sound marked as belonging to the interface alone when the game's clock slows.
	UGameplayStatics::PlaySound2D(this, Take, Volume, FMath::FRandRange(1.f - PitchSpread, 1.f + PitchSpread), 0.f, nullptr, nullptr, !bSlowsWithTheGame);
	return true;
}

// The film throws its own blood as each strike lands again, so it starts from a clean floor.
void AImpactEffects::HandleFinalCutStarted(const FTake& Take)
{
	bFilm = true;
	BrokenArmsHeard.Reset();
	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		Droplets[Index].bFlying = false;
		DropletMeshes[Index]->SetVisibility(false);
	}
}

void AImpactEffects::Spray(const FVector& From, const FVector& Direction, int32 Count, const FLinearColor& Color, bool bLeavesMarks, float FloorHeight)
{
	for (int32 Number = 0; Number < Count; ++Number)
	{
		const int32 Index = NextDroplet++ % PoolSize;

		FDroplet& Drop = Droplets[Index];
		Drop.Size = FMath::FRandRange(0.02f, 0.055f);
		Drop.Velocity = Direction * FMath::FRandRange(80.f, 380.f) + FMath::VRand() * 110.f + FVector(0.f, 0.f, FMath::FRandRange(60.f, 280.f));
		Drop.FloorHeight = FloorHeight;
		Drop.SecondsLeft = bLeavesMarks ? 10.f : FMath::FRandRange(0.15f, 0.3f);
		Drop.bLeavesMark = bLeavesMarks;
		Drop.bFlying = true;

		UStaticMeshComponent* Mesh = DropletMeshes[Index];
		Mesh->SetWorldLocation(From + FMath::VRand() * 4.f);
		Mesh->SetWorldScale3D(FVector(Drop.Size));
		Mesh->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Color);
		Mesh->SetVisibility(true);
	}
}

void AImpactEffects::SlowTime(float Dilation, float RealSeconds)
{
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
	SlowRealSecondsLeft = RealSeconds;
}
