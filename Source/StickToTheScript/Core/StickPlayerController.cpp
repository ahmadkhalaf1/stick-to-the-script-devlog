#include "StickToTheScript/Core/StickPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "StickToTheScript/Cam/BodyCam.h"
#include "StickToTheScript/Cam/FightCamera.h"
#include "StickToTheScript/Core/GameFlow.h"
#include "StickToTheScript/Core/StickGameInstance.h"
#include "StickToTheScript/Core/TakeDirector.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StickToTheScript/Fighter/StuntPartner.h"
#include "StickToTheScript/Fighter/SwingBar.h"
#include "StickToTheScript/Fighter/ThrownProp.h"

namespace
{
	// The player's strikes arrive fast, so a press and its impact read as one moment.
	const float PunchSeconds = 0.13f;
	const float KickSeconds = 0.22f;
	// One key pressed during a move is kept for the moment that move ends, the newest one: three strikes kept in a row
	// went on hitting long after the hands had stopped, and the fighter felt stuck in them. It is kept a little longer
	// than the longest a fighter is held by its own move, a roundhouse and what follows it.
	const float AskedForKeptSeconds = 0.55f;
	const int32 MostAskedFor = 1;
	// A strike key and a held key pressed together land a moment apart, in either order. Within this long they are one press.
	const float KeysTogetherSeconds = 0.09f;

	// A strike turns to face the nearest opponent inside this distance, so hits connect without careful aiming.
	const double AimAssistDistance = 170.;

	const float StickDeadZone = 0.25f;
	// The game's own player holds a gun this long before its first shot.
	const float AutoplayDrawSeconds = 0.6f;
	// It is there to show the fight, not to win it the quickest way. It fires a gun only once it has used its hands and
	// feet this often in a take, and then only this many times.
	const int32 AutoplayStrikesBeforeAGun = 5;
	const int32 AutoplayShotsATake = 3;
	const double AutoplayGunDetour = 380.;
	// After every so many strikes it stands still this long, as someone out of breath would, and the opponents get their
	// turn. What comes at it then is blocked this share of the times, and taken the rest. At five strikes, 1.9 s and a
	// half it lost the bar four times running.
	const int32 AutoplayStrikesBeforeARest = 6;
	const float AutoplayRestSeconds = 1.6f;
	const float AutoplayGuardShare = 0.7f;

	// What a blow that lands adds to the finish bar: about twenty blows fill it, fewer if they are big ones. At a dozen,
	// with everyone within 3.4 m ended by it, two finishes were a scene.
	const float FinishChargeForABlow = 0.05f;
	const float FinishChargeForABigBlow = 0.08f;
	// How long the low camera keeps the picture, by the clock on the wall: the kick in slow motion and what it does.
	const double FinishShotRealSeconds = 2.4;
	// The game's own player lets a full bar be seen for this long before it spends it.
	const float AutoplayFinishShownSeconds = 1.2f;
}

void AStickPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnAction.AddDynamic(this, &AStickPlayerController::HandleAction);
	Events->OnCut.AddDynamic(this, &AStickPlayerController::HandleCut);
	Events->OnTakeFinished.AddDynamic(this, &AStickPlayerController::HandleTakeFinished);
	Events->OnTakeStarted.AddDynamic(this, &AStickPlayerController::HandleTakeStarted);
	Events->OnHit.AddDynamic(this, &AStickPlayerController::HandleHit);
	Events->OnFinish.AddDynamic(this, &AStickPlayerController::HandleFinish);
}

void AStickPlayerController::HandleTakeStarted(int32 TakeNumber)
{
	FinishCharge = 0.f;
	EndFinishShot();
}

void AStickPlayerController::HandleHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	if (Attacker == Fighter && Attacker && GetWorld()->GetRealTimeSeconds() > FinishOwnBlowsUntilRealSeconds)
	{
		FinishCharge = FMath::Min(FinishCharge + (IsHeavyMove(Move) ? FinishChargeForABigBlow : FinishChargeForABlow), 1.f);
	}
}

void AStickPlayerController::TryFinish()
{
	if (!IsFighting() || FinishCharge < 1.f || !Fighter->Finish())
	{
		return;
	}
	FinishCharge = 0.f;
}

// Each blow of the finish gets its own picture, as a film would cut it: the hook from over the hero's shoulder, the
// uppercut from the floor, the kick from further back so that what it does is all in the frame.
void AStickPlayerController::HandleFinish(AActor* Attacker, int32 Beat)
{
	if (Attacker != Fighter || !Fighter)
	{
		return;
	}
	FinishOwnBlowsUntilRealSeconds = GetWorld()->GetRealTimeSeconds() + FinishShotRealSeconds;

	TArray<AActor*> Close;
	TArray<AActor*> Low;
	TArray<AActor*> Wide;
	UGameplayStatics::GetAllActorsWithTag(this, "Cam_Close", Close);
	UGameplayStatics::GetAllActorsWithTag(this, "Cam_Low", Low);
	UGameplayStatics::GetAllActorsWithTag(this, "Cam_Wide", Wide);
	if (Close.IsEmpty() || Low.IsEmpty() || Wide.IsEmpty())
	{
		return;
	}
	if (!FinishCamera)
	{
		ViewBeforeFinish = GetViewTarget();
	}

	const AStickFighter* Victim = NearestOpponent(UE_BIG_NUMBER);
	const FVector Hero = Fighter->GetJointPositions()[StickJoint::ChestJoint];
	const FVector Theirs = Victim ? Victim->GetJointPositions()[StickJoint::ChestJoint] : Hero + Fighter->GetActorForwardVector() * 80.;
	const FVector Pair = (Hero + Theirs) * 0.5;
	const FVector Side = -Wide[0]->GetActorForwardVector().GetSafeNormal2D();
	// From the hero towards whoever is being finished, flat along the fight.
	const FVector Onward = ((Theirs - Hero) * FVector(1., 1., 0.)).GetSafeNormal(UE_SMALL_NUMBER, Fighter->GetActorForwardVector());
	const double Floor = Fighter->GetActorLocation().Z;

	struct FAngle
	{
		bool bLow;
		double Out;
		double Along;
		double Height;
		double LookUp;
		float FieldOfView;
	};
	const FAngle Angles[] = {
		{ false, 250., -150., 135., 15., 50.f },
		{ true, 240., 70., 28., 55., 65.f },
		{ false, 430., 60., 80., 10., 62.f },
	};
	const FAngle& Angle = Angles[FMath::Clamp(Beat, 0, static_cast<int32>(UE_ARRAY_COUNT(Angles)) - 1)];
	const FVector Place = FVector(Pair.X, Pair.Y, Floor + Angle.Height) + Side * Angle.Out + Onward * Angle.Along;
	const FVector LookAt = Pair + FVector(0., 0., Angle.LookUp);
	FinishCamera = Angle.bLow ? Low[0] : Close[0];
	FinishCamera->SetActorLocationAndRotation(Place, (LookAt - Place).Rotation());
	if (AFightCamera* Lens = Cast<AFightCamera>(FinishCamera))
	{
		Lens->BaseFieldOfView = Angle.FieldOfView;
		Lens->FocusOn(Pair);
	}
	SetViewTarget(FinishCamera);
	FinishShotUntilRealSeconds = GetWorld()->GetRealTimeSeconds() + FinishShotRealSeconds;
}

void AStickPlayerController::EndFinishShot()
{
	// Only if the picture is still the finish's own: the film may have taken it since.
	if (FinishCamera && GetViewTarget() == FinishCamera && ViewBeforeFinish)
	{
		SetViewTarget(ViewBeforeFinish);
	}
	FinishCamera = nullptr;
	ViewBeforeFinish = nullptr;
	FinishShotUntilRealSeconds = 0.;
}

void AStickPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (FinishCamera && GetWorld()->GetRealTimeSeconds() > FinishShotUntilRealSeconds)
	{
		EndFinishShot();
	}

	if (!bFightLive || !Fighter || Fighter->IsDown())
	{
		return;
	}
	if (ATakeDirector::IsAutoplayOn())
	{
		PlayItself();
		return;
	}

	auto Held = [this](std::initializer_list<FKey> Keys)
	{
		for (const FKey& Key : Keys)
		{
			if (IsInputKeyDown(Key))
			{
				return true;
			}
		}
		return false;
	};
	auto Stick = [this](const FKey& Axis)
	{
		const float Value = GetInputAnalogKeyState(Axis);
		return FMath::Abs(Value) > StickDeadZone ? Value : 0.f;
	};

	const float Sideways = FMath::Clamp((Held({ EKeys::D, EKeys::Right }) ? 1.f : 0.f) - (Held({ EKeys::A, EKeys::Left }) ? 1.f : 0.f) + Stick(EKeys::Gamepad_LeftX), -1.f, 1.f);
	const float Depth = FMath::Clamp((Held({ EKeys::W, EKeys::Up }) ? 1.f : 0.f) - (Held({ EKeys::S, EKeys::Down }) ? 1.f : 0.f) + Stick(EKeys::Gamepad_LeftY), -1.f, 1.f);

	// Left and right are the camera's, so the controls stay the same however a stage is laid out.
	const FRotationMatrix View(FRotator(0., PlayerCameraManager->GetCameraRotation().Yaw, 0.));
	const FVector Right = View.GetUnitAxis(EAxis::Y);
	const FVector Away = View.GetUnitAxis(EAxis::X);
	Fighter->Walk(FVector2D(Right * Sideways + Away * Depth));

	// Seen from the side, a fighter with its back to the camera is hard to read, so it only ever turns left or right.
	if (FMath::Abs(Sideways) > 0.2f && !Fighter->IsStriking())
	{
		Fighter->FaceTowards(Fighter->GetActorLocation() + Right * (Sideways * 100.f));
	}

	// Putting the guard up or going down takes back the strikes still waiting: a defence is wanted now, not after them.
	// And a strike that began a moment before the key was meant to go with it.
	const EMove HeldNow = HeldKey();
	const bool bDuckHeld = HeldNow == EMove::Duck;
	const bool bGuardHeld = HeldNow == EMove::Block;
	const float Now = GetWorld()->GetTimeSeconds();
	if (HeldNow != EMove::None && !bWasDefending)
	{
		const bool bWaitingStrikeGoesWithIt = !AskedFor.IsEmpty() && AskedFor.Last().Move != EMove::None
			&& Now - (AskedFor.Last().Until - AskedForKeptSeconds) < KeysTogetherSeconds;
		if (bWaitingStrikeGoesWithIt)
		{
			FAskedFor Kept = AskedFor.Last();
			Kept.Held = HeldNow;
			AskedFor.Reset();
			AskedFor.Add(Kept);
		}
		else
		{
			AskedFor.Reset();
			Fighter->TurnStrikeInto(HeldNow);
		}
	}
	bWasDefending = HeldNow != EMove::None;

	// What was asked for too early is tried again every frame until it comes out or grows stale.
	while (!AskedFor.IsEmpty() && Now > AskedFor[0].Until)
	{
		AskedFor.RemoveAt(0);
	}
	if (!AskedFor.IsEmpty())
	{
		const EMove Next = AskedFor[0].Move;
		bool bDone = false;
		if (Next == EMove::None)
		{
			Fighter->Jump();
			bDone = Fighter->IsAirborne();
		}
		else
		{
			bDone = Fighter->Strike(Next, Next == EMove::Kick ? KickSeconds : PunchSeconds, AskedFor[0].Held);
		}
		if (bDone)
		{
			AskedFor.RemoveAt(0);
		}
	}

	// A gun goes on firing for as long as the punch is held. The gun itself says how fast.
	const AThrownProp* Gun = Fighter->GetCarried();
	if (Gun && Gun->IsGun() && Gun->GetShotsLeft() > 0 && Held({ EKeys::LeftMouseButton, EKeys::J, EKeys::Gamepad_FaceButton_Left }))
	{
		Fighter->StrikeWith(EMove::Punch, 0.f);
	}

	if (bDuckHeld)
	{
		Fighter->Guard(EMove::Duck);
	}
	else if (bGuardHeld)
	{
		Fighter->Guard(EMove::Block);
	}
}

void AStickPlayerController::SubmitMove(EMove Move)
{
	if (IsFighting())
	{
		if (!ATakeDirector::IsAutoplayOn() && (Move == EMove::Punch || Move == EMove::Kick))
		{
			if (const AStickFighter* Nearest = NearestOpponent(AimAssistDistance))
			{
				Fighter->FaceTowards(Nearest->GetActorLocation());
			}
			// The key held with it decides the move now, whatever the fighter is still in the middle of.
			const EMove HeldNow = HeldKey();
			// Behind anything already waiting, or it would jump the queue.
			if (!AskedFor.IsEmpty() || !Fighter->Strike(Move, Move == EMove::Kick ? KickSeconds : PunchSeconds, HeldNow))
			{
				AskFor(Move, HeldNow);
			}
		}
		return;
	}

	if (AGameFlow* Flow = Cast<AGameFlow>(UGameplayStatics::GetActorOfClass(this, AGameFlow::StaticClass())))
	{
		Flow->SubmitMove(Move);
	}
}

void AStickPlayerController::AskFor(EMove Move, EMove Held)
{
	// The newest key wins the last place: what the hands did last is what comes out.
	if (AskedFor.Num() >= MostAskedFor)
	{
		AskedFor.Pop();
	}
	AskedFor.Add({ Move, static_cast<float>(GetWorld()->GetTimeSeconds()) + AskedForKeptSeconds, Held });
}

void AStickPlayerController::CycleHeldForTests()
{
	HeldForTests = HeldForTests == EMove::None ? EMove::Duck : HeldForTests == EMove::Duck ? EMove::Block : EMove::None;
}

EMove AStickPlayerController::HeldKey() const
{
	if (HeldForTests != EMove::None)
	{
		return HeldForTests;
	}
	for (const FKey& Key : { EKeys::LeftControl, EKeys::RightControl, EKeys::C, EKeys::Gamepad_RightTrigger })
	{
		if (IsInputKeyDown(Key))
		{
			return EMove::Duck;
		}
	}
	for (const FKey& Key : { EKeys::RightMouseButton, EKeys::LeftShift, EKeys::RightShift, EKeys::L, EKeys::Gamepad_LeftTrigger })
	{
		if (IsInputKeyDown(Key))
		{
			return EMove::Block;
		}
	}
	return EMove::None;
}

void AStickPlayerController::JumpOrContinue()
{
	if (IsFighting())
	{
		if (!ATakeDirector::IsAutoplayOn())
		{
			const bool bWasUp = Fighter->IsAirborne() || Fighter->IsHanging();
			if (AskedFor.IsEmpty())
			{
				Fighter->Jump();
			}
			if (!bWasUp && !Fighter->IsAirborne())
			{
				AskFor(EMove::None);
			}
		}
	}
	else if (AGameFlow* Flow = Cast<AGameFlow>(UGameplayStatics::GetActorOfClass(this, AGameFlow::StaticClass())))
	{
		Flow->AnswerCard();
	}
}

void AStickPlayerController::GrabOrContinue()
{
	if (IsFighting())
	{
		if (!ATakeDirector::IsAutoplayOn())
		{
			Fighter->GrabOrThrow();
		}
	}
	else if (AGameFlow* Flow = Cast<AGameFlow>(UGameplayStatics::GetActorOfClass(this, AGameFlow::StaticClass())))
	{
		Flow->AnswerCard();
	}
}

void AStickPlayerController::ChangeLook(int32 Row, int32 Direction)
{
	if (AGameFlow* Flow = Cast<AGameFlow>(UGameplayStatics::GetActorOfClass(this, AGameFlow::StaticClass())))
	{
		Flow->ChangeLook(Row, Direction);
	}
}

bool AStickPlayerController::IsFighting() const
{
	return bFightLive && Fighter && !IsPaused();
}

void AStickPlayerController::TogglePause()
{
	if (AGameFlow* Flow = Cast<AGameFlow>(UGameplayStatics::GetActorOfClass(this, AGameFlow::StaticClass())))
	{
		Flow->TogglePause();
	}
}

void AStickPlayerController::ToggleView()
{
	TArray<AActor*> WideCameras;
	UGameplayStatics::GetAllActorsWithTag(this, "Cam_Wide", WideCameras);
	AActor* BodyCam = UGameplayStatics::GetActorOfClass(this, ABodyCam::StaticClass());

	AActor* Next = GetViewTarget() == BodyCam && !WideCameras.IsEmpty() ? WideCameras[0] : BodyCam;
	if (Next)
	{
		SetViewTarget(Next);
	}
}

// For demos, recordings and tests: the scene plays itself. On and off are separate keys, so a test always knows which it left.
void AStickPlayerController::SetAutoplay(bool bOn)
{
	if (IConsoleVariable* Autoplay = IConsoleManager::Get().FindConsoleVariable(TEXT("sts.Autoplay")))
	{
		Autoplay->Set(bOn ? 1 : 0);
	}
}

void AStickPlayerController::ToggleSlowFighters()
{
	bSlowFighters = !bSlowFighters;
	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		It->CustomTimeDilation = bSlowFighters ? 0.2f : 1.f;
	}
}

void AStickPlayerController::ToggleOpponentsHeld()
{
	bOpponentsHeld = !bOpponentsHeld;
	for (TActorIterator<AStuntPartner> It(GetWorld()); It; ++It)
	{
		It->CustomTimeDilation = bOpponentsHeld ? 0.02f : 1.f;
	}
}

void AStickPlayerController::JumpToScene(int32 Index)
{
	UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();
	FSceneScript Scene;
	if (!ATakeDirector::IsAutoplayOn() || !Game || !Game->LoadScene(Index, Scene))
	{
		return;
	}

	Game->SceneIndex = Index;
	Game->bRunStarted = true;
	Game->bDressed = true;
	UGameplayStatics::OpenLevel(this, FName(*Scene.Stage));
}

void AStickPlayerController::HandleAction()
{
	bFightLive = true;
	AutoplayStrikesThisTake = 0;
	AutoplayShotsThisTake = 0;
	AutoplayRestUntilSeconds = 0.f;
}

void AStickPlayerController::HandleCut(ECutReason Reason)
{
	bFightLive = false;
	if (Fighter)
	{
		Fighter->Walk(FVector2D::ZeroVector);
	}
}

void AStickPlayerController::HandleTakeFinished(const FTake& Take)
{
	HandleCut(ECutReason::KnockedOut);
}

AStickFighter* AStickPlayerController::NearestOpponent(double Within) const
{
	AStickFighter* Nearest = nullptr;
	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		const double Distance = FVector::Dist2D(It->GetActorLocation(), Fighter->GetActorLocation());
		if (It->Team != Fighter->Team && !It->IsDown() && Distance < Within)
		{
			Within = Distance;
			Nearest = *It;
		}
	}
	return Nearest;
}

// The autoplay fighter is no stylist: it walks up to whoever is nearest and hits them until they fall.
void AStickPlayerController::PlayItself()
{
	// It ducks when a shot is about to come.
	for (TActorIterator<AStuntPartner> It(GetWorld()); It; ++It)
	{
		if (It->IsAboutToFire())
		{
			Fighter->Walk(FVector2D::ZeroVector);
			Fighter->Guard(EMove::Duck);
			return;
		}
	}

	const AStickFighter* Target = NearestOpponent(UE_BIG_NUMBER);
	if (!Target)
	{
		Fighter->Walk(FVector2D::ZeroVector);
		return;
	}
	// A full bar is spent on whoever is near, once it has been on the screen long enough to be read.
	if (FinishCharge < 1.f)
	{
		AutoplayFinishFullSinceSeconds = -1.f;
	}
	else if (AutoplayFinishFullSinceSeconds < 0.f)
	{
		AutoplayFinishFullSinceSeconds = GetWorld()->GetTimeSeconds();
	}
	else if (GetWorld()->GetTimeSeconds() - AutoplayFinishFullSinceSeconds > AutoplayFinishShownSeconds)
	{
		TryFinish();
	}

	const FVector Mine = Fighter->GetActorLocation();
	const FVector Theirs = Target->GetActorLocation();

	// A bar overhead with someone in front of it is not passed up.
	if (Fighter->IsHanging())
	{
		Fighter->FaceTowards(Theirs);
		if (FMath::Abs(Theirs.X - Mine.X) < 195. && FMath::Abs(Theirs.Y - Mine.Y) < 55.)
		{
			Fighter->StrikeWith(EMove::Kick, 0.f);
		}
		return;
	}
	for (TActorIterator<ASwingBar> It(GetWorld()); It; ++It)
	{
		const double ToThem = FMath::Abs(Theirs.X - Mine.X);
		if (FMath::Abs(It->GetActorLocation().X - Mine.X) < 30. && ToThem > 90. && ToThem < 230. && Fighter->CanAct() && !Fighter->IsAirborne())
		{
			Fighter->Jump();
			return;
		}
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < AutoplayRestUntilSeconds)
	{
		Fighter->Walk(FVector2D::ZeroVector);
		if (!Fighter->IsStriking())
		{
			Fighter->FaceTowards(Theirs);
			if (bAutoplayGuards)
			{
				Fighter->Guard(EMove::Block);
			}
		}
		return;
	}

	// Once it has fought by hand for a while, a loaded gun lying near enough is worth a detour.
	const bool bMayShoot = AutoplayStrikesThisTake >= AutoplayStrikesBeforeAGun && AutoplayShotsThisTake < AutoplayShotsATake;
	if (bMayShoot && !Fighter->IsCarrying())
	{
		for (TActorIterator<AThrownProp> It(GetWorld()); It; ++It)
		{
			const FVector2D ToGun(It->GetActorLocation() - Mine);
			if (It->IsGun() && It->IsLoose() && It->GetShotsLeft() > 0 && ToGun.Size() < AutoplayGunDetour)
			{
				Fighter->Walk(ToGun.Size() > 40. ? ToGun.GetSafeNormal() : FVector2D::ZeroVector);
				if (Fighter->FindPropInReach() == *It)
				{
					Fighter->GrabOrThrow();
				}
				return;
			}
		}
	}

	const double Side = Mine.X <= Theirs.X ? -1. : 1.;
	const FVector2D Step = FVector2D(Theirs.X + Side * 90., Theirs.Y) - FVector2D(Mine);
	// It stops well inside its striking distance. Stopping at the edge of it left it facing a cornered gunman for good:
	// too far to strike by a hand's width, with an opponent who would not come closer.
	Fighter->Walk(Step.Size() > 8. ? Step.GetSafeNormal() : FVector2D::ZeroVector);

	if (!Fighter->IsStriking())
	{
		Fighter->FaceTowards(Theirs);
	}
	// It picks up whatever it passes, and throws it as soon as someone is lined up.
	const double Distance = FVector::Dist2D(Mine, Theirs);
	const AThrownProp* Held = Fighter->GetCarried();
	const bool bLoadedGun = bMayShoot && Held && Held->IsGun() && Held->GetShotsLeft() > 0;
	if (!bLoadedGun)
	{
		AutoplayUnarmedSeconds = Now;
	}
	if (bLoadedGun)
	{
		// A gun it may fire is fired at whoever is lined up, not thrown. Not in the first moment it is held:
		// the camera needs that long to bring whoever it points at into the picture.
		if (Now - AutoplayUnarmedSeconds > AutoplayDrawSeconds && Distance < 750. && FMath::Abs(Mine.Y - Theirs.Y) < 70. && Fighter->CanAct())
		{
			const int32 RoundsBefore = Held->GetShotsLeft();
			Fighter->StrikeWith(EMove::Punch, 0.f);
			AutoplayShotsThisTake += RoundsBefore - Held->GetShotsLeft();
		}
	}
	else if (Fighter->IsCarrying() ? Distance < 380. && FMath::Abs(Mine.Y - Theirs.Y) < 45. : Distance > 220.)
	{
		Fighter->GrabOrThrow();
	}
	if (Distance < 108. && Fighter->CanAct() && Held && Held->IsGun())
	{
		// At arm's length a gun is in the way of its fists, so it goes first: at whoever is in front of it.
		Fighter->GrabOrThrow();
	}
	else if (Distance < 108. && Fighter->CanAct())
	{
		// It goes through everything it knows in turn, so a run that plays itself shows every move.
		const EMove Moves[] = { EMove::Punch, EMove::Punch, EMove::Kick, EMove::Uppercut, EMove::Punch, EMove::Chop, EMove::Kick, EMove::Sweep, EMove::Punch, EMove::Roundhouse };
		const EMove Move = Moves[AutoplayStrikes++ % UE_ARRAY_COUNT(Moves)];
		if (Fighter->StrikeWith(Move, Move == EMove::Kick ? KickSeconds : Move == EMove::Punch ? PunchSeconds : 0.f) && ++AutoplayStrikesThisTake % AutoplayStrikesBeforeARest == 0)
		{
			AutoplayRestUntilSeconds = Now + AutoplayRestSeconds;
			bAutoplayGuards = FMath::FRand() < AutoplayGuardShare;
		}
	}
}

void AStickPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// The actions and their keys are built here, so no binary input asset has to be kept in step with the code.
	MoveMapping = NewObject<UInputMappingContext>(this);
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	auto MakeAction = [this](std::initializer_list<FKey> Keys)
	{
		UInputAction* Action = NewObject<UInputAction>(this);
		// Otherwise no key would reach the pause menu.
		Action->bTriggerWhenPaused = true;
		for (const FKey& Key : Keys)
		{
			MoveMapping->MapKey(Action, Key);
		}
		return Action;
	};

	// Walking, ducking and guarding are held keys, read every frame in PlayerTick.
	Input->BindAction(MakeAction({ EKeys::LeftMouseButton, EKeys::J, EKeys::Gamepad_FaceButton_Left }), ETriggerEvent::Started, this, &AStickPlayerController::SubmitMove, EMove::Punch);
	Input->BindAction(MakeAction({ EKeys::F, EKeys::K, EKeys::Gamepad_FaceButton_Top }), ETriggerEvent::Started, this, &AStickPlayerController::SubmitMove, EMove::Kick);
	Input->BindAction(MakeAction({ EKeys::SpaceBar, EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom }), ETriggerEvent::Started, this, &AStickPlayerController::JumpOrContinue);
	Input->BindAction(MakeAction({ EKeys::E, EKeys::Gamepad_FaceButton_Right }), ETriggerEvent::Started, this, &AStickPlayerController::GrabOrContinue);
	Input->BindAction(MakeAction({ EKeys::V }), ETriggerEvent::Started, this, &AStickPlayerController::ToggleView);
	Input->BindAction(MakeAction({ EKeys::Q, EKeys::Gamepad_RightShoulder }), ETriggerEvent::Started, this, &AStickPlayerController::TryFinish);
	// Escape ends a play session in the editor, so there is a second key.
	Input->BindAction(MakeAction({ EKeys::P, EKeys::Escape, EKeys::Gamepad_Special_Right }), ETriggerEvent::Started, this, &AStickPlayerController::TogglePause);

	// In the wardrobe the walking keys step through the choices. In a fight they are read as held keys, in PlayerTick.
	Input->BindAction(MakeAction({ EKeys::A, EKeys::Left }), ETriggerEvent::Started, this, &AStickPlayerController::ChangeLook, 0, -1);
	Input->BindAction(MakeAction({ EKeys::D, EKeys::Right }), ETriggerEvent::Started, this, &AStickPlayerController::ChangeLook, 0, 1);
	Input->BindAction(MakeAction({ EKeys::W, EKeys::Up }), ETriggerEvent::Started, this, &AStickPlayerController::ChangeLook, 1, 1);
	Input->BindAction(MakeAction({ EKeys::S, EKeys::Down }), ETriggerEvent::Started, this, &AStickPlayerController::ChangeLook, 1, -1);
	Input->BindAction(MakeAction({ EKeys::F9 }), ETriggerEvent::Started, this, &AStickPlayerController::SetAutoplay, true);
	Input->BindAction(MakeAction({ EKeys::F10 }), ETriggerEvent::Started, this, &AStickPlayerController::SetAutoplay, false);
	Input->BindAction(MakeAction({ EKeys::F8 }), ETriggerEvent::Started, this, &AStickPlayerController::ToggleSlowFighters);
#if WITH_EDITOR
	Input->BindAction(MakeAction({ EKeys::F7 }), ETriggerEvent::Started, this, &AStickPlayerController::ToggleOpponentsHeld);
	Input->BindAction(MakeAction({ EKeys::F6 }), ETriggerEvent::Started, this, &AStickPlayerController::CycleHeldForTests);
#endif
	Input->BindAction(MakeAction({ EKeys::One }), ETriggerEvent::Started, this, &AStickPlayerController::JumpToScene, 0);
	Input->BindAction(MakeAction({ EKeys::Two }), ETriggerEvent::Started, this, &AStickPlayerController::JumpToScene, 1);
	Input->BindAction(MakeAction({ EKeys::Three }), ETriggerEvent::Started, this, &AStickPlayerController::JumpToScene, 2);

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(MoveMapping, 0);
	}
}
