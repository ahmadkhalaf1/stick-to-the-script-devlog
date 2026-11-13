#include "StickToTheScript/Core/TakeDirector.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "StickToTheScript/Core/TakeEvents.h"

DEFINE_LOG_CATEGORY_STATIC(LogTakeDirector, Log, All);

static TAutoConsoleVariable<int32> CVarAutoplay(
	TEXT("sts.Autoplay"), 0,
	TEXT("1 makes the game play itself: the player's fighter fights by itself and the cards answer themselves."));

bool ATakeDirector::IsAutoplayOn()
{
	return CVarAutoplay.GetValueOnGameThread() != 0;
}

ATakeDirector::ATakeDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	Script.Name = INVTEXT("Scene 1");
}

void ATakeDirector::BeginPlay()
{
	Super::BeginPlay();
	Events().OnFighterDown.AddDynamic(this, &ATakeDirector::FighterWentDown);
}

void ATakeDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ATakeDirector::StartTake()
{
	const int32 Number = Take.Number + 1;
	Take = FTake();
	Take.Number = Number;
	OpponentsDown = 0;
	EnterPhase(ETakePhase::LeadIn, LeadInSeconds);
	Events().OnTakeStarted.Broadcast(Number);
}

void ATakeDirector::FighterWentDown(AActor* Fighter, bool bPlayer)
{
	if (Phase != ETakePhase::Fighting)
	{
		return;
	}

	if (bPlayer)
	{
		EnterPhase(ETakePhase::Cut, RetakeDelaySeconds);
		Events().OnCut.Broadcast(ECutReason::KnockedOut);
		return;
	}

	++OpponentsDown;
	UE_LOG(LogTakeDirector, Log, TEXT("Opponents down: %d of %d"), OpponentsDown, OpponentCount);
	if (OpponentsDown >= OpponentCount)
	{
		EnterPhase(ETakePhase::Wrap, WrapSeconds);
	}
}

void ATakeDirector::Advance(float DeltaSeconds)
{
	if (Phase == ETakePhase::Idle || Phase == ETakePhase::Finished)
	{
		return;
	}

	PhaseSecondsLeft -= DeltaSeconds;
	if (PhaseSecondsLeft > 0.f)
	{
		return;
	}

	switch (Phase)
	{
	case ETakePhase::LeadIn:
		EnterPhase(ETakePhase::Fighting, 0.f);
		Events().OnAction.Broadcast();
		break;
	case ETakePhase::Wrap:
		FinishTake();
		break;
	case ETakePhase::Cut:
		StartTake();
		break;
	default:
		break;
	}
}

UTakeEvents& ATakeDirector::Events() const
{
	UTakeEvents* TakeEvents = GetWorld()->GetSubsystem<UTakeEvents>();
	check(TakeEvents);
	return *TakeEvents;
}

void ATakeDirector::EnterPhase(ETakePhase NewPhase, float Seconds)
{
	Phase = NewPhase;
	PhaseSecondsLeft = Seconds;
}

void ATakeDirector::FinishTake()
{
	Take.bClean = true;
	EnterPhase(ETakePhase::Finished, 0.f);
	Events().OnTakeFinished.Broadcast(Take);
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTakeDirectorFreeFightTest, "StickToTheScript.Core.TakeDirector.FreeFightEndsWhenOneSideIsDown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTakeDirectorFreeFightTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	if (!TestNotNull(TEXT("There is a world to spawn the director in"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transient;
	ATakeDirector* Director = World->SpawnActor<ATakeDirector>(SpawnParameters);
	Director->OpponentCount = 2;
	// Every phase lasts one second, so Advance(1) always moves on exactly one phase.
	Director->LeadInSeconds = Director->RetakeDelaySeconds = Director->WrapSeconds = 1.f;

	Director->StartTake();
	TestEqual(TEXT("The first take is number 1"), Director->GetTakeNumber(), 1);
	Director->FighterWentDown(nullptr, true);
	TestTrue(TEXT("Nobody can fall before the fight is live"), Director->GetPhase() == ETakePhase::LeadIn);
	Director->Advance(1.f);
	TestTrue(TEXT("The lead-in ends with the fight live"), Director->GetPhase() == ETakePhase::Fighting);

	Director->FighterWentDown(nullptr, false);
	TestTrue(TEXT("One opponent down of two keeps the fight going"), Director->GetPhase() == ETakePhase::Fighting);
	Director->FighterWentDown(nullptr, true);
	TestTrue(TEXT("The player going down calls cut"), Director->GetPhase() == ETakePhase::Cut);
	Director->Advance(1.f);
	TestEqual(TEXT("The cut restarts as take 2"), Director->GetTakeNumber(), 2);

	Director->Advance(1.f);
	Director->FighterWentDown(nullptr, false);
	TestTrue(TEXT("A retake counts the opponents again from none"), Director->GetPhase() == ETakePhase::Fighting);
	Director->FighterWentDown(nullptr, false);
	TestTrue(TEXT("The last opponent down wraps the take"), Director->GetPhase() == ETakePhase::Wrap);
	Director->FighterWentDown(nullptr, true);
	TestTrue(TEXT("Falling after the wrap does not spoil it"), Director->GetPhase() == ETakePhase::Wrap);
	Director->Advance(1.f);
	TestTrue(TEXT("The wrap finishes the take"), Director->GetPhase() == ETakePhase::Finished);
	TestEqual(TEXT("The clean take keeps its number"), Director->GetTakeNumber(), 2);

	Director->Destroy();
	return true;
}

#endif
