#include "StickToTheScript/Cam/FinalCut.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "StickToTheScript/Cam/FightCamera.h"
#include "StickToTheScript/Cam/FightRecorder.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StickToTheScript/Fighter/ThrownProp.h"

namespace
{
	// What the film keeps around each kind of moment.
	const float OpeningSeconds = 1.2f;
	const float BigBeforeSeconds = 0.9f;
	// Long enough after a knockout for the body to reach the floor: cut sooner, the fall is left half done.
	const float BigAfterSeconds = 1.1f;
	const float LastBigAfterSeconds = 2.2f;
	const float MediumBeforeSeconds = 0.7f;
	const float MediumAfterSeconds = 0.6f;
	// A kick or a stopped blow is only added where the film has nothing within this long of it.
	const float MediumSpacingSeconds = 1.5f;
	// Two kept stretches closer than this run on as one, instead of cutting away and straight back.
	const float MergeGapSeconds = 0.5f;
	const float MaxFilmSeconds = 16.f;
	const float FilmShareOfTake = 0.42f;

	// The film slows down into the last knockout and into every thrown prop, and out of a thrown prop that knocked
	// nobody out. (Every knockout is slowed after it lands by the effects, the same as in play.)
	const float SlowBeforeSeconds = 0.45f;
	const float SlowAfterSeconds = 0.2f;
	const float SlowRate = 0.35f;
	const float SlowMotionCutRate = 0.25f;
	// A long take runs fast, as old fight films did: a minute of fight is a long time to watch at its own speed.
	const float LongTakeRate = 1.6f;

	// A close shot creeps in on what it looks at, so no shot in the film is a still picture.
	const float PushInSpeed = 22.f;
	const double LowCameraKeepInside = 60.;

	float TotalSeconds(const TArray<FFilmClip>& Clips)
	{
		float Total = 0.f;
		for (const FFilmClip& Clip : Clips)
		{
			Total += Clip.EndSeconds - Clip.StartSeconds;
		}
		return Total;
	}

	void SortAndMerge(TArray<FFilmClip>& Clips)
	{
		Clips.Sort([](const FFilmClip& A, const FFilmClip& B) { return A.StartSeconds < B.StartSeconds; });
		for (int32 Index = Clips.Num() - 1; Index > 0; --Index)
		{
			if (Clips[Index].StartSeconds <= Clips[Index - 1].EndSeconds + MergeGapSeconds)
			{
				Clips[Index - 1].EndSeconds = FMath::Max(Clips[Index - 1].EndSeconds, Clips[Index].EndSeconds);
				Clips.RemoveAt(Index);
			}
		}
	}
	// The film holds its last frame this long, under the slate.
	const float EndSlateSeconds = 2.6f;
	// An outtake stays on its last frame for a moment, under its "Cut!".
	const float OuttakeHoldSeconds = 0.6f;

	// A close-up starts just before its strike lands and stays a moment after.
	const float CutBeforeImpactSeconds = 0.35f;
	const float CutAfterShotSeconds = 0.25f;
	// How far to the side a close shot is taken from: far enough for two fighters and their heads.
	const double CloseUpDistance = 330.;
	const double LowDistance = 300.;
	const double CloseUpTowardsAttacker = 70.;
	const float CloseUpFollowSpeed = 6.f;
	const double PropHidesWithin = 55.;
	const float StayAfterImpactSeconds = 1.f;
	// In a long fight not every blow earns a cut. A knockout always does; the rest only after this long on other shots.
	const float SecondsBetweenCloseUps = 2.5f;

	const FName CameraTags[] = { "Cam_Wide", "Cam_Close", "Cam_Low" };
	const float CameraFieldsOfView[] = { 80.f, 55.f, 70.f };
}

AFinalCut::AFinalCut()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFinalCut::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UTakeEvents>()->OnTakeFinished.AddDynamic(this, &AFinalCut::HandleTakeFinished);
}

void AFinalCut::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	switch (Phase)
	{
	case EPhase::Titles:
		PhaseSecondsLeft -= DeltaSeconds;
		if (PhaseSecondsLeft <= 0.f)
		{
			Phase = EPhase::Playing;
			PlayingSinceRealSeconds = GetWorld()->GetRealTimeSeconds();
		}
		break;

	case EPhase::Playing:
		ReplaySeconds += DeltaSeconds * PlaybackRate(ReplaySeconds);
		if (ReplaySeconds >= Clips[ClipIndex].EndSeconds)
		{
			if (!Clips.IsValidIndex(ClipIndex + 1))
			{
				ShowMoment(Clips[ClipIndex].EndSeconds);
				Phase = EPhase::HoldingLastFrame;
				PhaseSecondsLeft = bOuttake ? OuttakeHoldSeconds : EndSlateSeconds;
				if (!bOuttake)
				{
					// By the clock on the wall: the frozen frames and the slow motion are part of what was watched.
					RunningSeconds = static_cast<float>(GetWorld()->GetRealTimeSeconds() - PlayingSinceRealSeconds);
				}
				break;
			}

			// A cut. The film leaves out what happened in between, blows included.
			ReplaySeconds = Clips[++ClipIndex].StartSeconds;
			while (NextEvent < Take.Events.Num() && Take.Events[NextEvent].TimeSeconds < ReplaySeconds)
			{
				++NextEvent;
			}
		}
		ShowMoment(ReplaySeconds);

		if (ShotCamera != WideCamera)
		{
			AActor* Camera = Cameras[ShotCamera];
			// The shot stays on the people in it. Held on the spot of the blow, it was left looking at a wall as soon as
			// someone was knocked across the room.
			if (Shots.IsValidIndex(CurrentShot))
			{
				ShotLookAt = FMath::VInterpTo(ShotLookAt, FindSubject(Shots[CurrentShot]), DeltaSeconds, CloseUpFollowSpeed);
				Camera->SetActorRotation((ShotLookAt - Camera->GetActorLocation()).Rotation());
			}
			Camera->AddActorWorldOffset((ShotLookAt - Camera->GetActorLocation()).GetSafeNormal() * PushInSpeed * DeltaSeconds);
			// Only the two cameras the film carries about get a lens. The wide one also films the fight, which stays sharp.
			if (AFightCamera* Lens = Cast<AFightCamera>(Camera))
			{
				Lens->FocusOn(ShotLookAt);
			}
		}
		break;

	case EPhase::HoldingLastFrame:
		PhaseSecondsLeft -= DeltaSeconds;
		if (PhaseSecondsLeft <= 0.f && !StartOuttake())
		{
			FinishReplay();
		}
		break;

	default:
		break;
	}
}

void AFinalCut::HandleTakeFinished(const FTake& FinishedTake)
{
	if (!Recorder)
	{
		return;
	}

	Take = Recorder->CloseTake(FinishedTake);
	if (Take.Frames.Num() < 2)
	{
		return;
	}

	Phase = EPhase::Ready;
}

void AFinalCut::Start(EFilmCut InCut)
{
	if (Phase != EPhase::Ready)
	{
		GetWorld()->GetSubsystem<UTakeEvents>()->OnFinalCutFinished.Broadcast();
		return;
	}

	Cut = InCut;
	StartReplay();
}

const TCHAR* AFinalCut::CutName(EFilmCut Cut)
{
	return Cut == EFilmCut::Action ? TEXT("The action cut") : Cut == EFilmCut::SlowMotion ? TEXT("The slow-motion cut") : TEXT("The long take");
}

void AFinalCut::StartReplay()
{
	FindOrPlaceCameras();
	PlanShots();

	Clips = PlanClips(Take.Events, Take.Frames.Last().TimeSeconds, Cut);
	ClipIndex = 0;
	ReplaySeconds = Clips[0].StartSeconds;
	Outtakes = Recorder->GetOuttakes();
	NextOuttake = 0;
	bOuttake = false;

	BigMoments.Reset();
	for (const FRecordedEvent& Event : Take.Events)
	{
		// A long take only slows for its ending.
		if (Event.Kind == ERecordedEventKind::Hit && (Event.bKnockout || ((Event.Move == EMove::Throw || Event.Move == EMove::SwingKick) && Cut != EFilmCut::LongTake)))
		{
			BigMoments.Add({ Event.TimeSeconds, Event.bKnockout });
		}
	}
	// Only the last knockout gets the slow run-up: six in a row would make the film crawl. The slow-motion cut is the
	// player asking for exactly that.
	const int32 LastKnockout = BigMoments.FindLastByPredicate([](const FBigMoment& Moment) { return Moment.bKnockout; });
	for (int32 Index = BigMoments.Num() - 1; Index >= 0; --Index)
	{
		if (BigMoments[Index].bKnockout && Index != LastKnockout && Cut != EFilmCut::SlowMotion)
		{
			BigMoments.RemoveAt(Index);
		}
	}

	FrameCursor = 0;
	CurrentShot = INDEX_NONE;
	NextEvent = 0;
	while (NextEvent < Take.Events.Num() && Take.Events[NextEvent].TimeSeconds < ReplaySeconds)
	{
		++NextEvent;
	}

	Phase = EPhase::Titles;
	PhaseSecondsLeft = TitleCardSeconds;
	RunningSeconds = 0.f;
	GetWorld()->GetSubsystem<UTakeEvents>()->OnFinalCutStarted.Broadcast(Take);
	ShowMoment(ReplaySeconds);
}

bool AFinalCut::StartOuttake()
{
	while (Outtakes.IsValidIndex(NextOuttake) && Outtakes[NextOuttake].Frames.Num() < 2)
	{
		++NextOuttake;
	}
	if (!Outtakes.IsValidIndex(NextOuttake))
	{
		return false;
	}

	// The same machinery as the film, on another take: all of what was kept of it, cut between the same cameras.
	Take = Outtakes[NextOuttake++];
	bOuttake = true;
	Clips = { { Take.Frames[0].TimeSeconds, Take.Frames.Last().TimeSeconds } };
	ClipIndex = 0;
	ReplaySeconds = Clips[0].StartSeconds;
	// At its own speed: a fall is funnier when it is not dwelt on.
	BigMoments.Reset();
	PlanShots();
	FrameCursor = 0;
	CurrentShot = INDEX_NONE;
	NextEvent = 0;

	Phase = EPhase::Playing;
	GetWorld()->GetSubsystem<UTakeEvents>()->OnOuttake.Broadcast(Take.Number, false);
	ShowMoment(ReplaySeconds);
	return true;
}

void AFinalCut::FinishReplay()
{
	for (int32 Index = 0; AStickFighter* Puppet = Recorder->GetFighter(Index); ++Index)
	{
		Puppet->EndPuppet();
	}

	if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
	{
		Player->SetViewTarget(Cameras[WideCamera]);
	}

	Phase = EPhase::Idle;
	GetWorld()->GetSubsystem<UTakeEvents>()->OnFinalCutFinished.Broadcast();
}

// A stage places cameras carrying the three tags. Where one is missing, a camera is put around the fight as it was recorded.
void AFinalCut::FindOrPlaceCameras()
{
	if (Cameras.Num() == CameraCount)
	{
		return;
	}

	const TArray<FFighterPose>& Fighters = Take.Frames[0].Fighters;
	const FVector Player = Fighters[0].Joints[StickJoint::PelvisJoint];
	const FVector Partner = Fighters.Num() > 1 ? Fighters[1].Joints[StickJoint::PelvisJoint] : Player + FVector(100., 0., 0.);
	const FVector Middle = (Player + Partner) * 0.5;
	const FVector Along = (Partner - Player).GetSafeNormal2D();
	const FVector Side = FVector::UpVector ^ Along;

	const FVector Places[] = {
		Middle + Side * 380.,
		// Close enough for both upper bodies, and to the side so neither fighter hides the strike.
		Middle + Side * 260. + FVector(0., 0., 50.),
		// Near the floor and a little behind the player, looking up past them at the opponent.
		Middle + Side * 170. - Along * 70. - FVector(0., 0., 70.),
	};

	Cameras.Reset();
	for (int32 Index = 0; Index < CameraCount; ++Index)
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(this, CameraTags[Index], Tagged);
		if (!Tagged.IsEmpty())
		{
			Cameras.Add(Tagged[0]);
			continue;
		}

		AFightCamera* Camera = GetWorld()->SpawnActor<AFightCamera>(Places[Index], (Middle - Places[Index]).Rotation());
		Camera->BaseFieldOfView = CameraFieldsOfView[Index];
		Camera->Tags.Add(CameraTags[Index]);
		Cameras.Add(Camera);
	}
}

void AFinalCut::PlanShots()
{
	Shots.Reset();
	Shots.AddDefaulted();
	if (Cut == EFilmCut::LongTake)
	{
		return;
	}

	float LastCloseUpSeconds = -UE_BIG_NUMBER;
	for (const FRecordedEvent& Event : Take.Events)
	{
		if (Event.Kind != ERecordedEventKind::Hit && Event.Kind != ERecordedEventKind::Blocked)
		{
			continue;
		}
		// A shot is watched from the wide camera, which holds the gun and whoever it points at in one picture: a
		// close-up of someone hit from outside it says nothing about where the shot came from. Only when a shot puts
		// someone down does the film cut in, and then after it has landed, to see the fall.
		// The same goes for something thrown: it arrives from wherever the thrower stood.
		const bool bShot = Event.Kind == ERecordedEventKind::Hit && (Event.Move == EMove::Shot || Event.Move == EMove::Blast || Event.Move == EMove::Throw);
		if (bShot && !Event.bKnockout)
		{
			continue;
		}
		if (!Event.bKnockout && Event.TimeSeconds < LastCloseUpSeconds + SecondsBetweenCloseUps)
		{
			continue;
		}
		LastCloseUpSeconds = Event.TimeSeconds;

		FShot CloseUp;
		CloseUp.StartSeconds = bShot ? Event.TimeSeconds + CutAfterShotSeconds : Event.TimeSeconds - CutBeforeImpactSeconds;
		// Heavy hits and the knockout are seen from the floor, looking up. Everything else gets the side close-up.
		const bool bBigMoment = Event.Kind == ERecordedEventKind::Hit && (Event.Move != EMove::Punch || Event.bKnockout);
		CloseUp.Camera = bBigMoment ? LowCamera : CloseCamera;
		CloseUp.LookAt = Event.ImpactPoint;
		CloseUp.Attacker = Event.AttackerIndex;
		CloseUp.Victim = Event.VictimIndex;

		// Two strikes close together share one run of close-ups: the return to the wide shot between them is dropped.
		if (Shots.Num() > 1 && Shots.Last().Camera == WideCamera && Shots.Last().StartSeconds > CloseUp.StartSeconds)
		{
			Shots.Pop();
		}
		Shots.Add(CloseUp);

		FShot BackToWide;
		BackToWide.StartSeconds = Event.TimeSeconds + StayAfterImpactSeconds;
		Shots.Add(BackToWide);
	}
}

void AFinalCut::ShowMoment(float Seconds)
{
	const TArray<FTakeFrame>& Frames = Take.Frames;
	while (FrameCursor + 1 < Frames.Num() && Frames[FrameCursor + 1].TimeSeconds <= Seconds)
	{
		++FrameCursor;
	}

	const FTakeFrame& From = Frames[FrameCursor];
	const FTakeFrame& To = Frames[FMath::Min(FrameCursor + 1, Frames.Num() - 1)];
	const float Span = To.TimeSeconds - From.TimeSeconds;
	const float Alpha = Span > UE_KINDA_SMALL_NUMBER ? FMath::Clamp((Seconds - From.TimeSeconds) / Span, 0.f, 1.f) : 0.f;

	for (int32 Index = 0; Index < From.Fighters.Num(); ++Index)
	{
		AStickFighter* Puppet = Recorder->GetFighter(Index);
		if (!Puppet)
		{
			continue;
		}

		FFighterPose Pose = From.Fighters[Index];
		const TArray<FVector>& Next = To.Fighters[Index].Joints;
		for (int32 Joint = 0; Joint < Pose.Joints.Num(); ++Joint)
		{
			Pose.Joints[Joint] = FMath::Lerp(Pose.Joints[Joint], Next[Joint], Alpha);
		}
		Puppet->SetPuppetPose(Pose);
	}

	for (int32 Index = 0; Index < FMath::Min(From.Props.Num(), To.Props.Num()); ++Index)
	{
		if (AThrownProp* Prop = Recorder->GetProp(Index))
		{
			FTransform Between;
			Between.Blend(From.Props[Index], To.Props[Index], Alpha);
			Prop->SetPuppetTransform(Between);
		}
	}

	while (NextEvent < Take.Events.Num() && Take.Events[NextEvent].TimeSeconds <= Seconds)
	{
		ReplayEvent(Take.Events[NextEvent++]);
	}

	int32 Shot = CurrentShot;
	while (Shots.IsValidIndex(Shot + 1) && Shots[Shot + 1].StartSeconds <= Seconds)
	{
		++Shot;
	}
	if (Shot != CurrentShot)
	{
		CurrentShot = Shot;
		CutTo(Shots[Shot]);
	}
}

// Firing the same events again is what brings back the blood, the shake, the frozen frame and the sound.
void AFinalCut::ReplayEvent(const FRecordedEvent& Event)
{
	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	if (Event.Kind == ERecordedEventKind::Cut)
	{
		// Only an outtake has one: it is where the director stopped the take.
		Events->OnOuttake.Broadcast(Take.Number, true);
		return;
	}

	AStickFighter* Attacker = Recorder->GetFighter(Event.AttackerIndex);
	AStickFighter* Victim = Recorder->GetFighter(Event.VictimIndex);
	if (!Attacker || !Victim)
	{
		return;
	}

	if (Event.Kind == ERecordedEventKind::Swing)
	{
		Events->OnSwing.Broadcast(Attacker, Event.Move);
	}
	else if (Event.Kind == ERecordedEventKind::Hit)
	{
		Victim->MarkStruck(Event.ImpactPoint, Event.bKnockout);
		Events->OnHit.Broadcast(Attacker, Victim, Event.Move);
	}
	else if (Event.Kind == ERecordedEventKind::Blocked)
	{
		Victim->MarkStruck(Event.ImpactPoint, false);
		Events->OnBlocked.Broadcast(Attacker, Victim);
	}
}

// What a close shot looks at: the chest of whoever is hit, drawn towards whoever hit them, so that one is more than an
// arm at the edge of the picture. From the chest and not from the spot of the blow, which keeps heads in the picture.
FVector AFinalCut::FindSubject(const FShot& Shot) const
{
	const AStickFighter* Victim = Recorder->GetFighter(Shot.Victim);
	if (!Victim)
	{
		return Shot.LookAt;
	}
	const FVector Struck = Victim->GetJointPositions()[StickJoint::ChestJoint];
	const AStickFighter* Attacker = Recorder->GetFighter(Shot.Attacker);
	if (!Attacker || Attacker == Victim)
	{
		return Struck;
	}
	return Struck + ((Attacker->GetJointPositions()[StickJoint::ChestJoint] - Struck) * 0.5).GetClampedToMaxSize(CloseUpTowardsAttacker);
}

// A prop standing between a camera and what it films fills half the picture with its back.
bool AFinalCut::IsHiddenByAProp(const FVector& From, const FVector& To) const
{
	const FVector Line = To - From;
	const double LengthSquared = FMath::Max(Line.SizeSquared(), 1.);
	for (int32 Index = 0; const AThrownProp* Prop = Recorder->GetProp(Index); ++Index)
	{
		const FVector At = Prop->GetActorLocation();
		const double Along = ((At - From) | Line) / LengthSquared;
		// What is at the far end is in somebody's hands or at their feet, and belongs in the shot.
		if (Along > 0.05 && Along < 0.8 && FVector::Dist(At, From + Line * Along) < PropHidesWithin)
		{
			return true;
		}
	}
	return false;
}

void AFinalCut::CutTo(const FShot& Shot)
{
	ShotCamera = Shot.Camera;
	ShotLookAt = Shot.Camera == WideCamera ? Shot.LookAt : FindSubject(Shot);
	AActor* Camera = Cameras[Shot.Camera];
	if (Shot.Camera != WideCamera)
	{
		// The fight travels, so these two cameras are carried to each strike: on the wide camera's side of the fight,
		// the close one level with the chest and the low one near the floor, a little behind the attacker. Far enough
		// back for both fighters and their heads.
		const AActor* Wide = Cameras[WideCamera];
		const FVector Side = -Wide->GetActorForwardVector().GetSafeNormal2D();
		const FVector Along = Wide->GetActorRightVector().GetSafeNormal2D();
		const AStickFighter* Player = Recorder->GetFighter(0);
		const double Floor = Player ? Player->GetActorLocation().Z : 0.;

		const FVector OnTheFloor(ShotLookAt.X, ShotLookAt.Y, Floor + 30.);
		const FVector Level = ShotLookAt + Side * CloseUpDistance + FVector(0., 0., 10.);
		// The first place a prop does not stand in front of. The last is taken whatever stands there.
		const FVector Places[] = {
			Shot.Camera == CloseCamera ? Level : OnTheFloor + Side * LowDistance - Along * 90.,
			Shot.Camera == CloseCamera ? Level + FVector(0., 0., 60.) : OnTheFloor + Side * LowDistance + Along * 90.,
			Level + FVector(0., 0., 60.) - Along * 120.,
			Level + FVector(0., 0., 60.) + Along * 120.,
		};
		FVector Place = Places[0];
		for (const FVector& Tried : Places)
		{
			if (!IsHiddenByAProp(Tried, ShotLookAt))
			{
				Place = Tried;
				break;
			}
		}
		if (Shot.Camera == LowCamera && Player)
		{
			// Furniture stands beyond the ends of the arena, and a camera on the floor there films the underside of a table.
			// shortcut: takes the fight to run along X, as every generated stage does. A stage laid out another way needs both axes.
			const double Left = Player->ClampToArena(FVector2D(-UE_BIG_NUMBER, Place.Y)).X + LowCameraKeepInside;
			const double Right = Player->ClampToArena(FVector2D(UE_BIG_NUMBER, Place.Y)).X - LowCameraKeepInside;
			if (Left < Right)
			{
				Place.X = FMath::Clamp(Place.X, Left, Right);
			}
		}
		Camera->SetActorLocationAndRotation(Place, (ShotLookAt - Place).Rotation());
	}

	if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
	{
		Player->SetViewTarget(Camera);
	}
}

void AFinalCut::Skip()
{
	if (Phase == EPhase::Titles || Phase == EPhase::Playing || Phase == EPhase::HoldingLastFrame)
	{
		FinishReplay();
	}
}

float AFinalCut::PlaybackRate(float Seconds) const
{
	if (bOuttake)
	{
		return 1.f;
	}
	for (const FBigMoment& Moment : BigMoments)
	{
		const float Since = Seconds - Moment.Seconds;
		if ((Since > -SlowBeforeSeconds && Since < 0.f) || (!Moment.bKnockout && Since >= 0.f && Since < SlowAfterSeconds))
		{
			return Cut == EFilmCut::SlowMotion ? SlowMotionCutRate : SlowRate;
		}
	}
	return Cut == EFilmCut::LongTake ? LongTakeRate : 1.f;
}

TArray<FFilmClip> AFinalCut::PlanClips(const TArray<FRecordedEvent>& Events, float TakeSeconds, EFilmCut Cut)
{
	TArray<float> Big;
	TArray<float> Medium;
	float FirstBlow = -1.f;
	for (const FRecordedEvent& Event : Events)
	{
		const bool bHit = Event.Kind == ERecordedEventKind::Hit;
		if (!bHit && Event.Kind != ERecordedEventKind::Blocked)
		{
			continue;
		}

		if (FirstBlow < 0.f)
		{
			FirstBlow = Event.TimeSeconds;
		}
		if (bHit && (Event.bKnockout || Event.Move == EMove::Throw || Event.Move == EMove::SwingKick))
		{
			Big.Add(Event.TimeSeconds);
		}
		else if (!bHit || Event.Move != EMove::Punch)
		{
			Medium.Add(Event.TimeSeconds);
		}
	}

	TArray<FFilmClip> Clips;
	auto Keep = [&Clips, TakeSeconds](float From, float To)
	{
		Clips.Add({ FMath::Max(From, 0.f), FMath::Min(To, TakeSeconds) });
		SortAndMerge(Clips);
	};

	// A take in which nobody landed anything is shown whole.
	if (FirstBlow < 0.f)
	{
		Keep(0.f, TakeSeconds);
		return Clips;
	}

	Keep(FirstBlow - OpeningSeconds, FirstBlow + MediumAfterSeconds);
	if (Cut == EFilmCut::LongTake)
	{
		return { { Clips[0].StartSeconds, TakeSeconds } };
	}
	for (int32 Index = 0; Index < Big.Num(); ++Index)
	{
		Keep(Big[Index] - BigBeforeSeconds, Big[Index] + (Index == Big.Num() - 1 ? LastBigAfterSeconds : BigAfterSeconds));
	}

	const float Budget = FMath::Min(MaxFilmSeconds, TakeSeconds * FilmShareOfTake);
	for (const float Moment : Medium)
	{
		const bool bAlreadyNear = Clips.ContainsByPredicate([Moment](const FFilmClip& Clip)
		{
			return Moment > Clip.StartSeconds - MediumSpacingSeconds && Moment < Clip.EndSeconds + MediumSpacingSeconds;
		});
		if (!bAlreadyNear && TotalSeconds(Clips) + MediumBeforeSeconds + MediumAfterSeconds <= Budget)
		{
			Keep(Moment - MediumBeforeSeconds, Moment + MediumAfterSeconds);
		}
	}
	return Clips;
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFinalCutClipsTest, "StickToTheScript.Cam.FinalCut.TheFilmKeepsTheHighlights",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFinalCutClipsTest::RunTest(const FString& Parameters)
{
	auto Blow = [](float Seconds, EMove Move, bool bKnockout)
	{
		FRecordedEvent Event;
		Event.Kind = ERecordedEventKind::Hit;
		Event.TimeSeconds = Seconds;
		Event.Move = Move;
		Event.bKnockout = bKnockout;
		return Event;
	};

	// A minute of fight: a punch every second, a kick every five, a thrown crate at 20 s, knockouts at 12, 33 and 58 s.
	TArray<FRecordedEvent> Events;
	for (int32 Second = 3; Second < 58; ++Second)
	{
		Events.Add(Blow(Second, Second % 5 == 0 ? EMove::Kick : EMove::Punch, false));
	}
	Events.Add(Blow(12.2f, EMove::Punch, true));
	Events.Add(Blow(20.4f, EMove::Throw, false));
	Events.Add(Blow(33.3f, EMove::Kick, true));
	Events.Add(Blow(58.f, EMove::Punch, true));
	Events.Sort([](const FRecordedEvent& A, const FRecordedEvent& B) { return A.TimeSeconds < B.TimeSeconds; });

	const float TakeSeconds = 60.f;
	const TArray<FFilmClip> Clips = AFinalCut::PlanClips(Events, TakeSeconds);

	auto Shows = [&Clips](float Seconds)
	{
		return Clips.ContainsByPredicate([Seconds](const FFilmClip& Clip) { return Seconds >= Clip.StartSeconds && Seconds <= Clip.EndSeconds; });
	};
	TestTrue(TEXT("The film opens on the first blow"), Shows(3.f) && Clips[0].StartSeconds < 3.f);
	TestTrue(TEXT("Every knockout is in the film"), Shows(12.2f) && Shows(33.3f) && Shows(58.f));
	TestTrue(TEXT("The thrown crate is in the film"), Shows(20.4f));
	TestTrue(TEXT("The film stays with the last knockout a while"), Clips.Last().EndSeconds >= 59.5f);

	float Total = 0.f;
	for (int32 Index = 0; Index < Clips.Num(); ++Index)
	{
		const FFilmClip& Clip = Clips[Index];
		Total += Clip.EndSeconds - Clip.StartSeconds;
		TestTrue(TEXT("A kept stretch has a length and lies inside the take"), Clip.StartSeconds >= 0.f && Clip.EndSeconds <= TakeSeconds && Clip.EndSeconds > Clip.StartSeconds);
		TestTrue(TEXT("Kept stretches are in order and apart"), Index == 0 || Clip.StartSeconds > Clips[Index - 1].EndSeconds);
	}
	TestTrue(TEXT("A minute of fight makes well under half a minute of film"), Total < 17.f);
	TestTrue(TEXT("... but more than only the knockouts"), Total > 10.f);

	const TArray<FFilmClip> Long = AFinalCut::PlanClips(Events, TakeSeconds, EFilmCut::LongTake);
	TestTrue(TEXT("A long take is one stretch, from just before the first blow to the end"),
		Long.Num() == 1 && Long[0].StartSeconds > 0.f && Long[0].StartSeconds < 3.f && Long[0].EndSeconds == TakeSeconds);
	TestTrue(TEXT("The slow-motion cut keeps what the action cut keeps"), AFinalCut::PlanClips(Events, TakeSeconds, EFilmCut::SlowMotion).Num() == Clips.Num());

	const TArray<FFilmClip> Quiet = AFinalCut::PlanClips({}, 9.f);
	TestTrue(TEXT("A take with no blows is shown whole"), Quiet.Num() == 1 && Quiet[0].StartSeconds == 0.f && Quiet[0].EndSeconds == 9.f);
	return true;
}

#endif
