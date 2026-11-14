#include "StickToTheScript/Core/TakeEvents.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Misc/OutputDeviceRedirector.h"

DEFINE_LOG_CATEGORY_STATIC(LogTakeEvents, Log, All);

void UTakeEvents::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	OnTakeStarted.AddDynamic(this, &UTakeEvents::LogTakeStarted);
	OnHit.AddDynamic(this, &UTakeEvents::LogHit);
	OnBlocked.AddDynamic(this, &UTakeEvents::LogBlocked);
	OnCut.AddDynamic(this, &UTakeEvents::LogCut);
	OnTakeFinished.AddDynamic(this, &UTakeEvents::LogTakeFinished);
	OnFinalCutStarted.AddDynamic(this, &UTakeEvents::LogFinalCutStarted);
	OnFinalCutFinished.AddDynamic(this, &UTakeEvents::LogFinalCutFinished);
	OnCard.AddDynamic(this, &UTakeEvents::LogCard);
	OnPayChanged.AddDynamic(this, &UTakeEvents::LogPayChanged);
	OnBoxOffice.AddDynamic(this, &UTakeEvents::LogBoxOffice);
	OnWardrobe.AddDynamic(this, &UTakeEvents::LogWardrobe);
	OnAction.AddDynamic(this, &UTakeEvents::LogAction);
	OnSwing.AddDynamic(this, &UTakeEvents::LogSwing);
	OnFighterDown.AddDynamic(this, &UTakeEvents::LogFighterDown);
	OnBarGrabbed.AddDynamic(this, &UTakeEvents::LogBarGrabbed);
	OnOuttake.AddDynamic(this, &UTakeEvents::LogOuttake);
}

void UTakeEvents::LogOuttake(int32 TakeNumber, bool bCut)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnOuttake: take %d%s"), TakeNumber, bCut ? TEXT(", cut") : TEXT(""));
}

void UTakeEvents::LogWardrobe(const FText& Choices)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnWardrobe: %s"), *Choices.ToString().Replace(TEXT("\n"), TEXT(" | ")));
}

void UTakeEvents::LogAction()
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnAction"));
}

void UTakeEvents::LogSwing(AActor* Attacker, EMove Move)
{
	UE_LOG(LogTakeEvents, Verbose, TEXT("OnSwing: %s throws %s"), *GetNameSafe(Attacker), *UEnum::GetValueAsString(Move));
}

void UTakeEvents::LogBarGrabbed(AActor* Fighter)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnBarGrabbed: %s"), *GetNameSafe(Fighter));
}

void UTakeEvents::LogFighterDown(AActor* Fighter, bool bPlayer)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnFighterDown: %s%s"), *GetNameSafe(Fighter), bPlayer ? TEXT(", the player") : TEXT(""));
}

void UTakeEvents::LogTakeStarted(int32 TakeNumber)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnTakeStarted: take %d"), TakeNumber);
}

void UTakeEvents::LogHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnHit: %s hit %s with %s"),
		*GetNameSafe(Attacker), *GetNameSafe(Victim), *UEnum::GetValueAsString(Move));
}

void UTakeEvents::LogBlocked(AActor* Attacker, AActor* Defender)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnBlocked: %s blocked %s"), *GetNameSafe(Defender), *GetNameSafe(Attacker));
}

void UTakeEvents::LogCut(ECutReason Reason)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnCut: %s"), *UEnum::GetValueAsString(Reason));
}

void UTakeEvents::LogTakeFinished(const FTake& Take)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnTakeFinished: take %d, %s, %d events, %d frames"),
		Take.Number, Take.bClean ? TEXT("clean") : TEXT("not clean"), Take.Events.Num(), Take.Frames.Num());
}

void UTakeEvents::LogFinalCutStarted(const FTake& Take)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnFinalCutStarted: take %d, %d frames, %d events"), Take.Number, Take.Frames.Num(), Take.Events.Num());
}

void UTakeEvents::LogFinalCutFinished()
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnFinalCutFinished"));
}

void UTakeEvents::LogCard(const FText& Headline, const FText& Detail)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnCard: \"%s\" %s"), *Headline.ToString(), *Detail.ToString().Replace(TEXT("\n"), TEXT(" | ")));
}

void UTakeEvents::LogBoxOffice(const FText& What, int32 Dollars)
{
	UE_LOG(LogTakeEvents, Verbose, TEXT("OnBoxOffice: $%d for %s"), Dollars, *What.ToString());
}

void UTakeEvents::LogPayChanged(int32 Pay)
{
	UE_LOG(LogTakeEvents, Log, TEXT("OnPayChanged: $%d"), Pay);
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTakeEventsLogTest, "StickToTheScript.Core.TakeEvents.EveryEventLogsOneLine",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTakeEventsLogTest::RunTest(const FString& Parameters)
{
	struct FTakeEventsLineCounter : FOutputDevice
	{
		int32 LineCount = 0;

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category == LogTakeEvents.GetCategoryName())
			{
				++LineCount;
			}
		}

		// The log only writes to a device on the calling thread, before Broadcast returns, if it claims this.
		virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	};

	UTakeEvents* Events = GWorld ? GWorld->GetSubsystem<UTakeEvents>() : nullptr;
	if (!TestNotNull(TEXT("The world has a UTakeEvents subsystem"), Events))
	{
		return false;
	}

	FTakeEventsLineCounter Counter;
	GLog->AddOutputDevice(&Counter);
	Events->OnTakeStarted.Broadcast(1);
	Events->OnHit.Broadcast(nullptr, nullptr, EMove::Punch);
	Events->OnBlocked.Broadcast(nullptr, nullptr);
	Events->OnCut.Broadcast(ECutReason::KnockedOut);
	Events->OnTakeFinished.Broadcast(FTake());
	Events->OnFinalCutStarted.Broadcast(FTake());
	Events->OnFinalCutFinished.Broadcast();
	Events->OnCard.Broadcast(FText(), FText());
	Events->OnPayChanged.Broadcast(0);
	Events->OnAction.Broadcast();
	Events->OnFighterDown.Broadcast(nullptr, false);
	Events->OnBarGrabbed.Broadcast(nullptr);
	Events->OnWardrobe.Broadcast(FText());
	Events->OnOuttake.Broadcast(1, false);
	GLog->RemoveOutputDevice(&Counter);

	return TestEqual(TEXT("Log lines printed by the fourteen events"), Counter.LineCount, 14);
}

#endif
