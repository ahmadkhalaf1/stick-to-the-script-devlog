#include "StickToTheScript/Core/GameFlow.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "StickToTheScript/Cam/FinalCut.h"
#include "StickToTheScript/Core/StickGameInstance.h"
#include "StickToTheScript/Core/TakeDirector.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StickToTheScript/Fighter/ThrownProp.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"

namespace
{
	const float SceneCardSeconds = 4.5f;
	const float PaidCardSeconds = 6.5f;
	const float WrapCardSeconds = 1.4f;
	// The player is often still hitting keys when the last opponent falls, and must not pick a cut by accident.
	const float CutChoiceGraceSeconds = 0.6f;
	// In radians: once round in about nine seconds.
	const float WardrobeTurnPerSecond = 0.7f;
	const int32 TakesAtFullStrength = 3;
	const float TiredPerTake = 0.15f;
	const float MostTired = 0.4f;
	const int32 CutCount = 3;
	const TCHAR* CutPromises[] = { TEXT("the best of it, fast"), TEXT("every knockout slowed right down"), TEXT("the whole fight, one camera, no cuts") };
	// Not in its first moments: the player is often still hitting keys when the last opponent falls.
	const float FilmCanBeSkippedAfterSeconds = 3.f;

	const int32 PunchPays = 2;
	const int32 KickPays = 5;
	const int32 ThrowPays = 30;
	const int32 AirPays = 20;
	const int32 KnockoutPays = 20;
	const int32 GuardPays = 10;
	const int32 VarietyPays = 40;
	// The same move pays this many times running. After that the audience has seen it.
	const int32 FreshInARow = 3;

	enum EShowKind : uint8
	{
		ShowPunch,
		ShowKick,
		ShowThrow,
		ShowAir,
		ShowGuard,
		ShowUppercut,
		ShowSweep,
		ShowRoundhouse,
		ShowChop,
		ShowBarKick,
		ShowShot
	};
	// Indexed by EShowKind. A guard is paid for in its own place.
	const int32 KindPays[] = { PunchPays, KickPays, ThrowPays, AirPays, GuardPays, 12, 8, 15, 8, 40, 6 };
	const TCHAR* KindNames[] = { TEXT("punch"), TEXT("kick"), TEXT("thrown"), TEXT("from the air"), TEXT("guard"),
		TEXT("uppercut"), TEXT("sweep"), TEXT("roundhouse"), TEXT("hook"), TEXT("off the bar"), TEXT("shot") };

	uint8 ShowKindOf(EMove Move, bool bFromTheAir)
	{
		switch (Move)
		{
		case EMove::SwingKick:
			return ShowBarKick;
		case EMove::Shot:
		case EMove::Blast:
			return ShowShot;
		case EMove::Throw:
			return ShowThrow;
		case EMove::Uppercut:
			return ShowUppercut;
		case EMove::Sweep:
			return ShowSweep;
		case EMove::Roundhouse:
			return ShowRoundhouse;
		case EMove::Chop:
			return ShowChop;
		default:
			return bFromTheAir ? ShowAir : Move == EMove::Punch ? ShowPunch : ShowKick;
		}
	}

	// With autoplay on, the cards that wait for a key answer themselves, so a whole run needs no input.
	const float AutoplayAnswersAfterSeconds = 3.5f;

	// For the results card: "scene 1 in 2, scene 3 in 5". A scene picked from the menu leaves the ones before it unshot.
	FString DescribeTakes(const TArray<int32>& TakesByScene)
	{
		TArray<FString> Parts;
		for (int32 Scene = 0; Scene < TakesByScene.Num(); ++Scene)
		{
			if (TakesByScene[Scene] > 0)
			{
				Parts.Add(FString::Printf(TEXT("scene %d in %d"), Scene + 1, TakesByScene[Scene]));
			}
		}
		return FString::Join(Parts, TEXT(", "));
	}
}

int32 FBoxOffice::Blow(EMove Move, bool bFromTheAir, bool bKnockout)
{
	const uint8 Kind = ShowKindOf(Move, bFromTheAir);
	SameInARow = Kind == LastKind ? SameInARow + 1 : 1;
	LastKind = Kind;

	int32 Pays = 0;
	if (SameInARow <= FreshInARow)
	{
		KindsSeen |= static_cast<uint16>(1 << Kind);
		Pays = KindPays[Kind];
		Thrown += Kind == ShowThrow ? 1 : 0;
		FromTheAir += Kind == ShowAir ? 1 : 0;
	}
	if (bKnockout)
	{
		++Knockouts;
		Pays += KnockoutPays;
	}

	Earned += Pays;
	return Pays;
}

int32 FBoxOffice::Guard()
{
	KindsSeen |= static_cast<uint16>(1 << ShowGuard);
	++Guards;
	Earned += GuardPays;
	return GuardPays;
}

int32 FBoxOffice::Variety() const
{
	return FMath::Max(static_cast<int32>(FMath::CountBits(KindsSeen)) - 1, 0) * VarietyPays;
}

FString FBoxOffice::Describe() const
{
	TArray<FString> Parts;
	auto Add = [&Parts](int32 Count, const TCHAR* One, const TCHAR* Many)
	{
		if (Count > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d %s"), Count, Count == 1 ? One : Many));
		}
	};
	Add(Knockouts, TEXT("knockout"), TEXT("knockouts"));
	Add(Thrown, TEXT("thrown"), TEXT("thrown"));
	Add(FromTheAir, TEXT("from the air"), TEXT("from the air"));
	Add(Guards, TEXT("guard"), TEXT("guards"));
	Add(static_cast<int32>(FMath::CountBits(KindsSeen)), TEXT("kind of move"), TEXT("kinds of move"));
	return FString::Join(Parts, TEXT(", "));
}

AGameFlow::AGameFlow()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGameFlow::BeginPlay()
{
	Super::BeginPlay();

	Director = Cast<ATakeDirector>(UGameplayStatics::GetActorOfClass(this, ATakeDirector::StaticClass()));

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnTakeStarted.AddDynamic(this, &AGameFlow::HandleTakeStarted);
	Events->OnTakeFinished.AddDynamic(this, &AGameFlow::HandleTakeFinished);
	Events->OnFinalCutFinished.AddDynamic(this, &AGameFlow::HandleFinalCutFinished);
	Events->OnHit.AddDynamic(this, &AGameFlow::HandleHit);
	Events->OnBlocked.AddDynamic(this, &AGameFlow::HandleBlocked);

	const UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();
	FSceneScript Scene;
	for (int32 Index = 0; Game && Game->LoadScene(Index, Scene); ++Index)
	{
		SceneNames.Add(Scene.Name.ToString());
	}

	if (Game && !Game->bRunStarted)
	{
		MenuCursor = IsUnlocked(Game->SceneIndex) ? Game->SceneIndex : 0;
		Enter(EStep::Title);
		ShowMenu();
	}
	else if (Game && !Game->bDressed)
	{
		// A scene picked from the menu opens its own level, and the wardrobe was not seen on the way.
		Enter(EStep::Wardrobe);
		ShowWardrobe();
	}
	else
	{
		Enter(EStep::SceneCard, SceneCardSeconds);
	}
}

void AGameFlow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	StepSeconds += DeltaSeconds;
	if (WaitsForKey())
	{
		// In the wardrobe the fighter turns on the spot, so what is being chosen is seen from every side.
		if (AStickFighter* Fighter = Step == EStep::Wardrobe ? FindPlayerFighter() : nullptr)
		{
			const float Angle = StepSeconds * WardrobeTurnPerSecond;
			Fighter->FaceTowards(Fighter->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.) * 100.);
		}
		if (ATakeDirector::IsAutoplayOn() && StepSeconds > AutoplayAnswersAfterSeconds)
		{
			Continue();
		}
		return;
	}

	if (Step == EStep::SceneCard || Step == EStep::Paid || Step == EStep::Wrapped)
	{
		StepSecondsLeft -= DeltaSeconds;
		if (StepSecondsLeft <= 0.f)
		{
			Continue();
		}
	}
}

void AGameFlow::ChangeLook(int32 Row, int32 Direction)
{
	if (bPaused || Step == EStep::Title || Step == EStep::Cutting || Step == EStep::Shop)
	{
		// Up is the hat's "next" key, and the row above on a menu.
		if (Row == 1 && MenuRows > 0)
		{
			MenuCursor = (MenuCursor - Direction + MenuRows) % MenuRows;
			ShowMenu();
		}
		else if (Row == 0 && MenuRows > 0)
		{
			// Left and right too: the first menu lays its scenes out side by side.
			MenuCursor = (MenuCursor + Direction + MenuRows) % MenuRows;
			ShowMenu();
		}
		return;
	}

	UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();
	if (Step != EStep::Wardrobe || !Game)
	{
		return;
	}

	int32& Choice = Row == 0 ? Game->Look.Color : Row == 1 ? Game->Look.Hat : Game->Look.Build;
	const int32 Count = AStickFighter::LookCount(Row);
	// What has to be bought first is passed over. The first entry of every row is free, so this ends.
	do
	{
		Choice = (Choice + Direction + Count) % Count;
	}
	while (!Game->OwnsLook(Row, Choice));

	if (AStickFighter* Fighter = FindPlayerFighter())
	{
		Fighter->ApplyLook(Game->Look);
	}
	ShowWardrobe();
}

void AGameFlow::PointAtRow(int32 Row)
{
	const bool bMenu = bPaused || Step == EStep::Title || Step == EStep::Cutting || Step == EStep::Shop;
	if (bMenu && Row >= 0 && Row < MenuRows && Row != MenuCursor)
	{
		MenuCursor = Row;
		ShowMenu();
	}
}

void AGameFlow::PointAtLook(int32 Row, int32 Index)
{
	PointedLookRow = Row;
	PointedLookIndex = Index;
}

void AGameFlow::ShowWardrobe()
{
	const UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();
	if (!Game)
	{
		return;
	}

	const FCharacterLook& Look = Game->Look;
	GetWorld()->GetSubsystem<UTakeEvents>()->OnWardrobe.Broadcast(FText::FromString(FString::Printf(
		TEXT("Colour:   <  %s  >      A / D\nHat:   <  %s  >      W / S\nBuild:   <  %s  >      F\n\nThis is how you will look in the film. Left click when you are ready."),
		*AStickFighter::LookName(0, Look.Color), *AStickFighter::LookName(1, Look.Hat), *AStickFighter::LookName(2, Look.Build))));
}

// The player's fighter is the one on the player's side that fights. A captive is on that side too, and does not.
AStickFighter* AGameFlow::FindPlayerFighter() const
{
	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		if (It->Team == 0 && It->FightsFreely())
		{
			return *It;
		}
	}
	return nullptr;
}

void AGameFlow::AnswerCard()
{
	if (bPaused)
	{
		ChooseFromMenu();
	}
	else if (WaitsForKey())
	{
		Continue();
	}
	else
	{
		SkipFilm();
	}
}

void AGameFlow::SkipFilm()
{
	if (Step != EStep::Film || StepSeconds < FilmCanBeSkippedAfterSeconds)
	{
		return;
	}
	if (AFinalCut* Film = Cast<AFinalCut>(UGameplayStatics::GetActorOfClass(this, AFinalCut::StaticClass())))
	{
		Film->Skip();
	}
}

void AGameFlow::SubmitMove(EMove Move)
{
	if (bPaused)
	{
		ChooseFromMenu();
	}
	else if (Step == EStep::Wardrobe && Move == EMove::Kick)
	{
		ChangeLook(2, 1);
	}
	else if (Step == EStep::Wardrobe && Move == EMove::Punch && PointedLookRow != INDEX_NONE)
	{
		// A click on a colour, a hat or a build takes it. A click anywhere else says the fighter is ready.
		UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();
		if (Game && Game->OwnsLook(PointedLookRow, PointedLookIndex))
		{
			(PointedLookRow == 0 ? Game->Look.Color : PointedLookRow == 1 ? Game->Look.Hat : Game->Look.Build) = PointedLookIndex;
			if (AStickFighter* Fighter = FindPlayerFighter())
			{
				Fighter->ApplyLook(Game->Look);
			}
			ShowWardrobe();
		}
	}
	else if (WaitsForKey())
	{
		Continue();
	}
	else
	{
		SkipFilm();
	}
}

void AGameFlow::HandleTakeStarted(int32 TakeNumber)
{
	CurrentTake = TakeNumber;
	BoxOffice = FBoxOffice();
	GetWorld()->GetSubsystem<UTakeEvents>()->OnPayChanged.Broadcast(PayAfter(TakeNumber));
	if (AStickFighter* Player = FindPlayerFighter())
	{
		Player->DamageTakenScale = DamageShareInTake(TakeNumber);
	}
}

float AGameFlow::DamageShareInTake(int32 TakeNumber)
{
	return FMath::Max(1.f - TiredPerTake * FMath::Max(TakeNumber - TakesAtFullStrength, 0), MostTired);
}

void AGameFlow::HandleHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	const AStickFighter* Striker = Cast<AStickFighter>(Attacker);
	const AStickFighter* Struck = Cast<AStickFighter>(Victim);
	// Only the shoot counts. The film lands the same blows again, and they were paid for the first time.
	if (Step != EStep::Shooting || !Striker || !Struck || Striker->Team != 0)
	{
		return;
	}

	const bool bFromTheAir = Striker->IsAirborne();
	const bool bKnockout = Struck->IsDown();
	const int32 Pays = BoxOffice.Blow(Move, bFromTheAir, bKnockout);
	Announce(bKnockout ? TEXT("knockout") : KindNames[ShowKindOf(Move, bFromTheAir)], Pays);
}

void AGameFlow::HandleBlocked(AActor* Attacker, AActor* Defender)
{
	const AStickFighter* Guard = Cast<AStickFighter>(Defender);
	if (Step == EStep::Shooting && Guard && Guard->Team == 0)
	{
		Announce(TEXT("guard"), BoxOffice.Guard());
	}
}

void AGameFlow::Announce(const TCHAR* What, int32 Dollars)
{
	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnBoxOffice.Broadcast(FText::FromString(Dollars > 0 ? What : TEXT("seen it before")), Dollars);
	Events->OnPayChanged.Broadcast(PayAfter(CurrentTake) + BoxOffice.Total());
}

void AGameFlow::HandleTakeFinished(const FTake& Take)
{
	TakesUsed = Take.Number;
	Enter(EStep::Wrapped, WrapCardSeconds);
}

void AGameFlow::HandleFinalCutFinished()
{
	const int32 Fee = Director ? Director->Script.Fee : 0;
	const int32 FeePaid = PayAfter(TakesUsed);
	const int32 Show = BoxOffice.Total();
	if (UStickGameInstance* Game = GetGameInstance<UStickGameInstance>())
	{
		Game->Earned += FeePaid + Show;
		Game->Bank(FeePaid + Show);
		Game->BoxOffice += Show;
		Game->Offered += Fee;
		Game->TakesByScene.SetNumZeroed(FMath::Max(Game->TakesByScene.Num(), Game->SceneIndex + 1));
		Game->TakesByScene[Game->SceneIndex] = TakesUsed;
	}

	const int32 Retakes = TakesUsed - 1;
	const FString FeeLine = Retakes == 0
		? FString::Printf(TEXT("First take: the full fee, $%d."), Fee)
		: FString::Printf(TEXT("Fee $%d, less %d retake%s: $%d."), Fee, Retakes, Retakes == 1 ? TEXT("") : TEXT("s"), FeePaid);
	ShowCard(TEXT("SCENE WRAPPED"), FString::Printf(TEXT("%s\nBox office: $%d\n%s\n\nYou made $%d."), *FeeLine, Show, *BoxOffice.Describe(), FeePaid + Show));
	Enter(EStep::Paid, PaidCardSeconds);
}

void AGameFlow::Enter(EStep NewStep, float Seconds)
{
	Step = NewStep;
	StepSeconds = 0.f;
	StepSecondsLeft = Seconds;

	if (Step == EStep::SceneCard && Director)
	{
		const FSceneScript& Script = Director->Script;
		const FString Pay = FString::Printf(TEXT("Fee $%d. Every retake costs $%d.\nThe box office pays extra for a good show: mix your moves."), Script.Fee, Script.CutCost);
		const FString Written = Script.Synopsis.IsEmpty() ? FString() : FString::Printf(TEXT("THE SCRIPT SAYS:\n%s\nYou have other ideas.\n\n"), *Script.Synopsis.ToString());
		const FString Weapon = BringBoughtWeapon();
		ShowCard(Script.Name.ToString(), Written + Pay + (Weapon.IsEmpty() ? FString() : FString::Printf(TEXT("\nYour %s is at your feet. Pick it up with E."), *Weapon.ToLower())));
	}
}

// Moves on from whatever is on screen: a card that waited for a key, or one whose time is up.
void AGameFlow::Continue()
{
	UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();

	switch (Step)
	{
	case EStep::Title:
		ChooseFromMenu();
		break;

	case EStep::Credits:
		Enter(EStep::Title);
		ShowMenu();
		break;

	case EStep::Shop:
		ChooseFromMenu();
		break;

	case EStep::Wrapped:
		// The test fighter asks for a different cut in each scene, so a run that plays itself shows all three.
		MenuCursor = ATakeDirector::IsAutoplayOn() && Game ? Game->SceneIndex % CutCount : 0;
		Enter(EStep::Cutting);
		ShowMenu();
		break;

	case EStep::Cutting:
		ChooseFromMenu();
		break;

	case EStep::Wardrobe:
		if (Game)
		{
			Game->bDressed = true;
		}
		GetWorld()->GetSubsystem<UTakeEvents>()->OnWardrobe.Broadcast(FText());
		Enter(EStep::SceneCard, SceneCardSeconds);
		break;

	case EStep::SceneCard:
		ShowCard(FString(), FString());
		Enter(EStep::Shooting);
		if (Director)
		{
			Director->StartTake();
		}
		break;

	case EStep::Paid:
	{
		FSceneScript Next;
		if (Game && Game->LoadScene(Game->SceneIndex + 1, Next))
		{
			++Game->SceneIndex;
			OpenStage(Next.Stage);
		}
		else if (Game)
		{
			ShowCard(TEXT("THAT'S THE PICTURE"),
				(Director && !Director->Script.Epilogue.IsEmpty() ? Director->Script.Epilogue.ToString() + TEXT("\n\n") : FString())
						+ FString::Printf(TEXT("Fees: $%d of the $%d offered.\nBox office: $%d.\nTakes: %s.\nYou made $%d.\n\n>\tBack to the menu\n\nEnter or click"), Game->Earned - Game->BoxOffice, Game->Offered, Game->BoxOffice, *DescribeTakes(Game->TakesByScene), Game->Earned));
			Enter(EStep::Results);
		}
		else
		{
			Enter(EStep::SceneCard, SceneCardSeconds);
		}
		break;
	}

	case EStep::Results:
		BackToMenu();
		break;

	default:
		break;
	}
}

bool AGameFlow::WaitsForKey() const
{
	return Step == EStep::Title || Step == EStep::Wardrobe || Step == EStep::Results || Step == EStep::Credits || Step == EStep::Cutting || Step == EStep::Shop;
}

// With autoplay on nothing is locked, so a test can start at any scene.
bool AGameFlow::IsUnlocked(int32 Scene) const
{
	const UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();
	return ATakeDirector::IsAutoplayOn() || (Game && Scene <= Game->ScenesWrapped);
}

void AGameFlow::ShowMenu()
{
	const UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();

	TArray<FString> Rows;
	FString Detail;
	const bool bCutting = !bPaused && Step == EStep::Cutting;
	const bool bShop = !bPaused && Step == EStep::Shop;
	if (bPaused)
	{
		Rows = { TEXT("Resume"), TEXT("Back to the menu"), TEXT("Quit") };
	}
	else if (bCutting)
	{
		for (int32 Cut = 0; Cut < CutCount; ++Cut)
		{
			Rows.Add(FString::Printf(TEXT("%s: %s"), AFinalCut::CutName(static_cast<EFilmCut>(Cut)), CutPromises[Cut]));
		}
		Detail = TEXT("The director has walked off the set. The cut is yours.\n\n");
	}
	else if (bShop)
	{
		for (int32 Item = 0; Game && Item < UStickGameInstance::ShopItemCount(); ++Item)
		{
			Rows.Add(FString::Printf(TEXT("%s     %s"), *UStickGameInstance::ShopItemName(Item), *Game->ShopItemState(Item)));
		}
		Rows.Add(TEXT("Back"));
		Detail = FString::Printf(TEXT("In the bank: $%d\n%s\n"), Game ? Game->Banked : 0, *ShopNote);
	}
	else
	{
		for (int32 Scene = 0; Scene < SceneNames.Num(); ++Scene)
		{
			Rows.Add(IsUnlocked(Scene) ? SceneNames[Scene] : FString::Printf(TEXT("Scene %d: locked"), Scene + 1));
		}
		Rows.Add(TEXT("Shop"));
		Rows.Add(TEXT("Credits"));
		Rows.Add(TEXT("Quit"));

		Detail = TEXT("The script says you lose every fight. You have other ideas.\n");
		if (Game && Game->Banked > 0)
		{
			Detail += FString::Printf(TEXT("In the bank: $%d"), Game->Banked);
		}
		Detail += TEXT("\n");
	}
	MenuRows = Rows.Num();

	for (int32 Row = 0; Row < Rows.Num(); ++Row)
	{
		// The HUD draws a line that starts with a tab as a row to choose, and one with this mark before it as the chosen row.
		Detail += (Row == MenuCursor ? TEXT(">\t") : TEXT("\t")) + Rows[Row] + TEXT("\n");
	}
	if (bPaused)
	{
		Detail += TEXT("\nLeft click: punch      Right click: guard      F: kick      Ctrl: duck      Space: jump      E: grab, throw\nDuck and punch: uppercut      Duck and kick: sweep      Guard and punch: hook      Guard and kick: roundhouse\nJump under a red bar to hang from it, then kick      A gun in your hands: left click fires it\n");
	}
	Detail += bShop ? TEXT("\nW / S: choose      Enter or click: buy")
		: bPaused || bCutting ? TEXT("\nW / S: choose      Enter or click: go")
		: TEXT("\nA / D  or  W / S: choose      Enter or click: play      Esc: pause");
	ShowCard(bPaused ? TEXT("PAUSED") : bCutting ? TEXT("DIRECTOR'S CUT") : bShop ? TEXT("THE PROP SHOP") : TEXT("STICK TO THE SCRIPT"), Detail);
}

void AGameFlow::ChooseFromMenu()
{
	UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();

	if (bPaused)
	{
		if (MenuCursor == 0)
		{
			TogglePause();
		}
		else if (MenuCursor == 1)
		{
			BackToMenu();
		}
		else
		{
			UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
		}
	}
	else if (Step == EStep::Cutting)
	{
		AFinalCut* Film = Cast<AFinalCut>(UGameplayStatics::GetActorOfClass(this, AFinalCut::StaticClass()));
		if (StepSeconds < CutChoiceGraceSeconds || !Film)
		{
			return;
		}
		ShowCard(FString(), FString());
		Enter(EStep::Film);
		Film->Start(static_cast<EFilmCut>(MenuCursor));
	}
	else if (Step == EStep::Shop)
	{
		if (!Game || MenuCursor >= UStickGameInstance::ShopItemCount())
		{
			MenuCursor = SceneNames.Num();
			Enter(EStep::Title);
		}
		else if (Game->Owns(MenuCursor))
		{
			ShopNote = TEXT("That is yours already.");
		}
		else if (UStickGameInstance::IsWeapon(MenuCursor) && Game->BoughtWeapon != INDEX_NONE)
		{
			ShopNote = TEXT("One weapon at a time. Use the one that is waiting first.");
		}
		else
		{
			ShopNote = Game->Buy(MenuCursor) ? TEXT("Sold.") : TEXT("Not enough in the bank.");
		}
		ShowMenu();
	}
	else if (MenuCursor == SceneNames.Num())
	{
		MenuCursor = 0;
		ShopNote = TEXT("A weapon is yours for the next scene you shoot. A hat or a colour is yours for good.");
		Enter(EStep::Shop);
		ShowMenu();
	}
	else if (MenuCursor == SceneNames.Num() + 1)
	{
		ShowCard(TEXT("CREDITS"),
			TEXT("Made for the 2026 Epic AI MegaJam.\nDirected by one human. Built with an AI coding agent, Claude Code.\nUnreal Engine 5.8. The fighters' body and their moves are Mixamo's, posed on sixteen joints.\nThe sets are built by scripts. Their surfaces, their sky, the guns and the props are Poly Haven's scans.\nThe director's voice, the blows and the music: ElevenLabs, through fal.ai.\n\nNo stunt performers were paid what the script promised.\n\n>\tBack\n\nEnter or click"));
		Enter(EStep::Credits);
	}
	else if (MenuCursor > SceneNames.Num() + 1)
	{
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
	}
	else if (Game && IsUnlocked(MenuCursor))
	{
		Game->bRunStarted = true;
		if (MenuCursor != Game->SceneIndex)
		{
			FSceneScript Scene;
			Game->SceneIndex = MenuCursor;
			Game->LoadScene(MenuCursor, Scene);
			OpenStage(Scene.Stage);
			return;
		}
		ShowCard(FString(), FString());
		Enter(EStep::Wardrobe);
		ShowWardrobe();
	}
}

// A weapon bought in the shop lies at the player's feet for this scene, every take of it, and with that it is used up.
// Its name comes back for the scene card.
FString AGameFlow::BringBoughtWeapon()
{
	UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();
	const AStickFighter* Player = FindPlayerFighter();
	if (!Game || !Player || Game->BoughtWeapon == INDEX_NONE)
	{
		return FString();
	}

	for (TActorIterator<AThrownProp> It(GetWorld()); It; ++It)
	{
		if (It->bShopSlot)
		{
			It->BringOnSet(static_cast<EPropKind>(Game->BoughtWeapon), FVector2D(Player->GetActorLocation() + Player->GetActorForwardVector() * 70.));
			for (int32 Item = 0; Item < UStickGameInstance::ShopItemCount(); ++Item)
			{
				if (Game->Owns(Item) && UStickGameInstance::IsWeapon(Item))
				{
					const FString Name = UStickGameInstance::ShopItemName(Item);
					Game->UseUpWeapon();
					return Name;
				}
			}
		}
	}
	return FString();
}

void AGameFlow::BackToMenu()
{
	FSceneScript First;
	if (UStickGameInstance* Game = GetGameInstance<UStickGameInstance>())
	{
		Game->StartOver();
		Game->LoadScene(0, First);
	}
	UGameplayStatics::SetGamePaused(this, false);
	OpenStage(First.Stage);
}

void AGameFlow::TogglePause()
{
	if (!bPaused && Step != EStep::Shooting && Step != EStep::Film)
	{
		return;
	}

	bPaused = !bPaused;
	UGameplayStatics::SetGamePaused(this, bPaused);
	MenuCursor = 0;
	if (bPaused)
	{
		ShowMenu();
	}
	else
	{
		ShowCard(FString(), FString());
	}
}

void AGameFlow::ShowCard(const FString& Headline, const FString& Detail)
{
	GetWorld()->GetSubsystem<UTakeEvents>()->OnCard.Broadcast(FText::FromString(Headline), FText::FromString(Detail));
}

// An empty stage means the scene is shot where the last one was, on a fresh copy of that level.
void AGameFlow::OpenStage(const FString& StagePackage)
{
	const FString Level = StagePackage.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(this) : StagePackage;
	UGameplayStatics::OpenLevel(this, FName(*Level));
}

int32 AGameFlow::PayAfter(int32 Takes) const
{
	return Director ? FMath::Max(Director->Script.Fee - (Takes - 1) * Director->Script.CutCost, 0) : 0;
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoxOfficeTest, "StickToTheScript.Core.GameFlow.BoxOfficePaysForVariety",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBoxOfficeTest::RunTest(const FString& Parameters)
{
	FBoxOffice Show;
	TestEqual(TEXT("A punch pays"), Show.Blow(EMove::Punch, false, false), PunchPays);
	Show.Blow(EMove::Punch, false, false);
	Show.Blow(EMove::Punch, false, false);
	TestEqual(TEXT("The fourth punch running pays nothing"), Show.Blow(EMove::Punch, false, false), 0);
	TestEqual(TEXT("... unless it is the knockout"), Show.Blow(EMove::Punch, false, true), KnockoutPays);
	TestEqual(TEXT("A kick is something new"), Show.Blow(EMove::Kick, false, false), KickPays);
	TestEqual(TEXT("... and a punch after it pays again"), Show.Blow(EMove::Punch, false, false), PunchPays);
	TestEqual(TEXT("A thrown prop pays most"), Show.Blow(EMove::Throw, false, false), ThrowPays);
	TestEqual(TEXT("Any blow from the air pays as one"), Show.Blow(EMove::Kick, true, false), AirPays);
	TestEqual(TEXT("A guard that stops a blow pays"), Show.Guard(), GuardPays);

	TestEqual(TEXT("Five kinds of move earn four variety bonuses"), Show.Variety(), 4 * VarietyPays);
	TestEqual(TEXT("The total is what was earned plus the variety"), Show.Total(),
		4 * PunchPays + KnockoutPays + KickPays + ThrowPays + AirPays + GuardPays + 4 * VarietyPays);
	TestEqual(TEXT("The pay card counts what happened"), Show.Describe(), FString(TEXT("1 knockout, 1 thrown, 1 from the air, 1 guard, 5 kinds of move")));

	TestEqual(TEXT("The results name the takes of the scenes that were shot"), DescribeTakes({ 2, 0, 5 }), FString(TEXT("scene 1 in 2, scene 3 in 5")));

	FBoxOffice Fancy;
	TestEqual(TEXT("An uppercut pays as its own kind"), Fancy.Blow(EMove::Uppercut, false, false), 12);
	TestEqual(TEXT("... and so does a roundhouse"), Fancy.Blow(EMove::Roundhouse, false, false), 15);
	TestEqual(TEXT("Two kinds of move earn one variety bonus"), Fancy.Variety(), VarietyPays);

	FBoxOffice Dull;
	Dull.Blow(EMove::Punch, false, false);
	TestEqual(TEXT("One kind of move earns no variety bonus"), Dull.Variety(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetakesTireTest, "StickToTheScript.Core.GameFlow.RetakesTireTheOpponents",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRetakesTireTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("The first take is fought at full strength"), AGameFlow::DamageShareInTake(1), 1.f);
	TestEqual(TEXT("... and so is the third"), AGameFlow::DamageShareInTake(3), 1.f);
	TestTrue(TEXT("The fourth take hurts less"), AGameFlow::DamageShareInTake(4) < 1.f);
	TestTrue(TEXT("... and the fifth less again"), AGameFlow::DamageShareInTake(5) < AGameFlow::DamageShareInTake(4));
	TestEqual(TEXT("It stops getting easier"), AGameFlow::DamageShareInTake(50), AGameFlow::DamageShareInTake(51));
	TestTrue(TEXT("... well short of harmless"), AGameFlow::DamageShareInTake(50) >= 0.4f);
	return true;
}

#endif
