#include "StickToTheScript/Cam/FightRecorder.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StickToTheScript/Fighter/ThrownProp.h"

DEFINE_LOG_CATEGORY_STATIC(LogFightRecorder, Log, All);

namespace
{
	// An outtake is the end of a take that was cut: this long of the fight before the "Cut!", and this long of the fall after.
	const float OuttakeBeforeCutSeconds = 2.f;
	const float OuttakeAfterCutSeconds = 1.4f;
	const int32 MostOuttakesKept = 3;
}

AFightRecorder::AFightRecorder()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFightRecorder::BeginPlay()
{
	Super::BeginPlay();

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnTakeStarted.AddDynamic(this, &AFightRecorder::HandleTakeStarted);
	Events->OnHit.AddDynamic(this, &AFightRecorder::HandleHit);
	Events->OnBlocked.AddDynamic(this, &AFightRecorder::HandleBlocked);
	Events->OnSwing.AddDynamic(this, &AFightRecorder::HandleSwing);
	Events->OnCut.AddDynamic(this, &AFightRecorder::HandleCut);
	Events->OnTakeFinished.AddDynamic(this, &AFightRecorder::HandleTakeFinished);
}

void AFightRecorder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRecording)
	{
		return;
	}

	TArray<FFighterPose> Poses;
	Poses.Reserve(Fighters.Num());
	for (const AStickFighter* Fighter : Fighters)
	{
		FFighterPose& Pose = Poses.AddDefaulted_GetRef();
		Pose.Joints = Fighter->GetJointPositions();
		Pose.BrokenArms = Fighter->GetBrokenArms();
	}
	AddFrame(TakeSeconds(), Poses);

	TArray<FTransform>& PropTransforms = Recording.Frames.Last().Props;
	for (const AThrownProp* Prop : Props)
	{
		PropTransforms.Add(Prop->GetActorTransform());
	}

	if (StopAtSeconds >= 0.f && TakeSeconds() >= StopAtSeconds)
	{
		KeepOuttake();
	}
}

void AFightRecorder::Watch(const TArray<AStickFighter*>& FightersInOrder)
{
	Fighters.Reset();
	for (AStickFighter* Fighter : FightersInOrder)
	{
		Fighters.Add(Fighter);
		// A frame must hold where the fighters are this tick, not where they were last tick.
		AddTickPrerequisiteActor(Fighter);
	}
}

void AFightRecorder::BeginTake(int32 Number)
{
	Recording = FTake();
	StopAtSeconds = -1.f;
	Recording.Number = Number;
	for (const AStickFighter* Fighter : Fighters)
	{
		Recording.FighterColors.Add(Fighter->Color);
	}
	bRecording = true;
}

void AFightRecorder::AddFrame(float TimeSeconds, const TArray<FFighterPose>& Poses)
{
	FTakeFrame& Frame = Recording.Frames.AddDefaulted_GetRef();
	Frame.TimeSeconds = TimeSeconds;
	Frame.Fighters = Poses;
}

const FTake& AFightRecorder::CloseTake(const FTake& FromDirector)
{
	Recording.Number = FromDirector.Number;
	Recording.bClean = FromDirector.bClean;
	return Recording;
}

AStickFighter* AFightRecorder::GetFighter(int32 Index) const
{
	return Fighters.IsValidIndex(Index) ? Fighters[Index].Get() : nullptr;
}

void AFightRecorder::WatchProps(const TArray<AThrownProp*>& PropsInOrder)
{
	Props.Reset();
	for (AThrownProp* Prop : PropsInOrder)
	{
		Props.Add(Prop);
		AddTickPrerequisiteActor(Prop);
	}
}

AThrownProp* AFightRecorder::GetProp(int32 Index) const
{
	return Props.IsValidIndex(Index) ? Props[Index].Get() : nullptr;
}

void AFightRecorder::KeepOuttake()
{
	StopAndReport();
	StopAtSeconds = -1.f;
	if (Recording.Frames.IsEmpty())
	{
		return;
	}

	const float From = Recording.Frames.Last().TimeSeconds - OuttakeBeforeCutSeconds - OuttakeAfterCutSeconds;
	FTake Outtake;
	Outtake.Number = Recording.Number;
	Outtake.FighterColors = Recording.FighterColors;
	Outtake.Frames = Recording.Frames.FilterByPredicate([From](const FTakeFrame& Frame) { return Frame.TimeSeconds >= From; });
	Outtake.Events = Recording.Events.FilterByPredicate([From](const FRecordedEvent& Event) { return Event.TimeSeconds >= From; });
	Outtakes.Add(MoveTemp(Outtake));
	if (Outtakes.Num() > MostOuttakesKept)
	{
		Outtakes.RemoveAt(0);
	}
}

void AFightRecorder::HandleTakeStarted(int32 TakeNumber)
{
	// The next take can be called before the fall of the last one has run out.
	if (bRecording && StopAtSeconds >= 0.f)
	{
		KeepOuttake();
	}
	TakeStartSeconds = GetWorld()->GetTimeSeconds();
	BeginTake(TakeNumber);
}

void AFightRecorder::HandleHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	const AStickFighter* Struck = Cast<AStickFighter>(Victim);
	if (bRecording && Struck)
	{
		FRecordedEvent& Event = AddEvent(ERecordedEventKind::Hit);
		Event.AttackerIndex = IndexOf(Attacker);
		Event.VictimIndex = IndexOf(Victim);
		Event.Move = Move;
		Event.ImpactPoint = Struck->GetLastImpactPoint();
		Event.bKnockout = Struck->IsDown();
	}
}

void AFightRecorder::HandleSwing(AActor* Attacker, EMove Move)
{
	// Kept whether or not it lands: the whoosh of a strike, and the flash and noise of a gun, happen either way.
	if (bRecording)
	{
		FRecordedEvent& Event = AddEvent(ERecordedEventKind::Swing);
		Event.AttackerIndex = IndexOf(Attacker);
		Event.VictimIndex = Event.AttackerIndex;
		Event.Move = Move;
	}
}

void AFightRecorder::HandleBlocked(AActor* Attacker, AActor* Defender)
{
	const AStickFighter* Guard = Cast<AStickFighter>(Defender);
	if (bRecording && Guard)
	{
		FRecordedEvent& Event = AddEvent(ERecordedEventKind::Blocked);
		Event.AttackerIndex = IndexOf(Attacker);
		Event.VictimIndex = IndexOf(Defender);
		Event.ImpactPoint = Guard->GetLastImpactPoint();
	}
}

void AFightRecorder::HandleCut(ECutReason Reason)
{
	if (bRecording)
	{
		AddEvent(ERecordedEventKind::Cut).CutReason = Reason;
		StopAtSeconds = TakeSeconds() + OuttakeAfterCutSeconds;
	}
}

void AFightRecorder::HandleTakeFinished(const FTake& Take)
{
	if (bRecording)
	{
		StopAndReport();
	}
}

FRecordedEvent& AFightRecorder::AddEvent(ERecordedEventKind Kind)
{
	FRecordedEvent& Event = Recording.Events.AddDefaulted_GetRef();
	Event.Kind = Kind;
	Event.TimeSeconds = TakeSeconds();
	return Event;
}

int32 AFightRecorder::IndexOf(const AActor* Fighter) const
{
	return Fighters.IndexOfByPredicate([Fighter](const TObjectPtr<AStickFighter>& Watched) { return Watched.Get() == Fighter; });
}

float AFightRecorder::TakeSeconds() const
{
	return GetWorld()->GetTimeSeconds() - TakeStartSeconds;
}

void AFightRecorder::StopAndReport()
{
	bRecording = false;

	const int32 Frames = Recording.Frames.Num();
	const float Seconds = Frames > 0 ? Recording.Frames.Last().TimeSeconds : 0.f;
	if (Frames == 0 || Seconds <= 0.f)
	{
		return;
	}

	const SIZE_T BytesPerFrame = sizeof(FTakeFrame) + Fighters.Num() * (sizeof(FFighterPose) + StickJoint::JointCount * sizeof(FVector));
	const double Megabytes = static_cast<double>(Frames) * BytesPerFrame / (1024. * 1024.);
	UE_LOG(LogFightRecorder, Log, TEXT("Take %d: %d frames and %d events over %.1f s use %.2f MB. A 60-second take at this rate would use %.1f MB."),
		Recording.Number, Frames, Recording.Events.Num(), Seconds, Megabytes, Megabytes * 60. / Seconds);
	// Where everyone was when it stopped: a body that has left the set shows up here as a number.
	for (const AStickFighter* Fighter : Fighters)
	{
		const FVector Hips = Fighter->GetJointPositions()[StickJoint::PelvisJoint];
		UE_LOG(LogFightRecorder, Log, TEXT("  %s ends take %d at %.0f, %.0f, %.0f"), *Fighter->GetName(), Recording.Number, Hips.X, Hips.Y, Hips.Z);
	}
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightRecorderRoundTripTest, "StickToTheScript.Cam.FightRecorder.FramesReadBackAsRecorded",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFightRecorderRoundTripTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	if (!TestNotNull(TEXT("There is a world to spawn the recorder in"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transient;
	AFightRecorder* Recorder = World->SpawnActor<AFightRecorder>(SpawnParameters);

	Recorder->BeginTake(7);
	for (int32 FrameIndex = 0; FrameIndex < 10; ++FrameIndex)
	{
		FFighterPose Pose;
		for (int32 Joint = 0; Joint < StickJoint::JointCount; ++Joint)
		{
			Pose.Joints.Add(FVector(FrameIndex, Joint, FrameIndex * Joint));
		}
		Pose.BrokenArms = FrameIndex >= 5 ? 2 : 0;
		Recorder->AddFrame(FrameIndex * 0.1f, { Pose, Pose });
	}

	FTake FromDirector;
	FromDirector.Number = 7;
	FromDirector.bClean = true;
	const FTake& Take = Recorder->CloseTake(FromDirector);

	TestEqual(TEXT("Ten frames were kept"), Take.Frames.Num(), 10);
	TestEqual(TEXT("Each frame holds both fighters"), Take.Frames[9].Fighters.Num(), 2);
	TestEqual(TEXT("A fighter has every joint"), Take.Frames[9].Fighters[1].Joints.Num(), static_cast<int32>(StickJoint::JointCount));
	TestTrue(TEXT("A joint reads back where it was recorded"), Take.Frames[9].Fighters[1].Joints[3] == FVector(9., 3., 27.));
	TestTrue(TEXT("Frame times read back"), FMath::IsNearlyEqual(Take.Frames[4].TimeSeconds, 0.4f));
	TestEqual(TEXT("An arm is unbroken before frame 5"), static_cast<int32>(Take.Frames[4].Fighters[0].BrokenArms), 0);
	TestEqual(TEXT("The right arm is broken from frame 5"), static_cast<int32>(Take.Frames[5].Fighters[0].BrokenArms), 2);
	TestTrue(TEXT("The director's verdict is copied in"), Take.bClean && Take.Number == 7);

	// Four takes of five seconds that were cut: an outtake is the last 3.4 s of one, and only the latest three are kept.
	for (int32 Number = 1; Number <= 4; ++Number)
	{
		Recorder->BeginTake(Number);
		for (int32 FrameIndex = 0; FrameIndex <= 20; ++FrameIndex)
		{
			Recorder->AddFrame(FrameIndex * 0.25f, {});
		}
		Recorder->KeepOuttake();
	}
	const TArray<FTake>& Outtakes = Recorder->GetOuttakes();
	TestEqual(TEXT("Only the latest three outtakes are kept"), Outtakes.Num(), 3);
	TestTrue(TEXT("They are in the order they were shot"), Outtakes.Num() == 3 && Outtakes[0].Number == 2 && Outtakes[2].Number == 4);
	TestTrue(TEXT("An outtake is the end of its take"), Outtakes.Num() == 3 && Outtakes[0].Frames.Num() == 14
		&& FMath::IsNearlyEqual(Outtakes[0].Frames[0].TimeSeconds, 1.75f, 0.01f) && FMath::IsNearlyEqual(Outtakes[0].Frames.Last().TimeSeconds, 5.f, 0.01f));

	Recorder->Destroy();
	return true;
}

#endif
