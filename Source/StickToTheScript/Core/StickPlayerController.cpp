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
	const float PunchSeconds = 0.16f;
	const float KickSeconds = 0.26f;
	// Longer than the longest a fighter is held by its own move: a kick and its recovery.
	const float AskedForKeptSeconds = 0.75f;
	const int32 MostAskedFor = 3;

	// A strike turns to face the nearest opponent inside this distance, so hits connect without careful aiming.
	const double AimAssistDistance = 170.;

	const float StickDeadZone = 0.25f;
}

void AStickPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnAction.AddDynamic(this, &AStickPlayerController::HandleAction);
	Events->OnCut.AddDynamic(this, &AStickPlayerController::HandleCut);
	Events->OnTakeFinished.AddDynamic(this, &AStickPlayerController::HandleTakeFinished);
}

void AStickPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

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

	// What was asked for too early is tried again every frame until it comes out or grows stale.
	const float Now = GetWorld()->GetTimeSeconds();
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
			bDone = Fighter->Strike(Next, Next == EMove::Kick ? KickSeconds : PunchSeconds);
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

	if (Held({ EKeys::LeftControl, EKeys::RightControl, EKeys::C, EKeys::Gamepad_RightTrigger }))
	{
		Fighter->Guard(EMove::Duck);
	}
	else if (Held({ EKeys::RightMouseButton, EKeys::LeftShift, EKeys::RightShift, EKeys::L, EKeys::Gamepad_LeftTrigger }))
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
			// Behind anything already waiting, or it would jump the queue.
			if (!AskedFor.IsEmpty() || !Fighter->Strike(Move, Move == EMove::Kick ? KickSeconds : PunchSeconds))
			{
				AskFor(Move);
			}
		}
		return;
	}

	if (AGameFlow* Flow = Cast<AGameFlow>(UGameplayStatics::GetActorOfClass(this, AGameFlow::StaticClass())))
	{
		Flow->SubmitMove(Move);
	}
}

void AStickPlayerController::AskFor(EMove Move)
{
	if (AskedFor.Num() < MostAskedFor)
	{
		AskedFor.Add({ Move, static_cast<float>(GetWorld()->GetTimeSeconds()) + AskedForKeptSeconds });
	}
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
	if (Held && Held->IsGun() && Held->GetShotsLeft() > 0)
	{
		// A gun with rounds in it is fired at whoever is lined up, not thrown.
		if (Distance < 750. && FMath::Abs(Mine.Y - Theirs.Y) < 70. && Fighter->CanAct())
		{
			Fighter->StrikeWith(EMove::Punch, 0.f);
		}
	}
	else if (Fighter->IsCarrying() ? Distance < 380. && FMath::Abs(Mine.Y - Theirs.Y) < 45. : Distance > 220.)
	{
		Fighter->GrabOrThrow();
	}
	if (Distance < 108. && Fighter->CanAct())
	{
		// It goes through everything it knows in turn, so a run that plays itself shows every move.
		const EMove Moves[] = { EMove::Punch, EMove::Punch, EMove::Kick, EMove::Uppercut, EMove::Punch, EMove::Chop, EMove::Kick, EMove::Sweep, EMove::Punch, EMove::Roundhouse };
		const EMove Move = Moves[AutoplayStrikes++ % UE_ARRAY_COUNT(Moves)];
		Fighter->StrikeWith(Move, Move == EMove::Kick ? KickSeconds : Move == EMove::Punch ? PunchSeconds : 0.f);
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
	Input->BindAction(MakeAction({ EKeys::One }), ETriggerEvent::Started, this, &AStickPlayerController::JumpToScene, 0);
	Input->BindAction(MakeAction({ EKeys::Two }), ETriggerEvent::Started, this, &AStickPlayerController::JumpToScene, 1);
	Input->BindAction(MakeAction({ EKeys::Three }), ETriggerEvent::Started, this, &AStickPlayerController::JumpToScene, 2);

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(MoveMapping, 0);
	}
}
