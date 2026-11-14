#include "StickToTheScript/UI/StickHUD.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "StickToTheScript/Cam/FinalCut.h"
#include "StickToTheScript/Core/GameFlow.h"
#include "StickToTheScript/Core/StickGameInstance.h"
#include "StickToTheScript/Core/TakeDirector.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StickToTheScript/Fighter/ThrownProp.h"
#include "StickToTheScript/UI/ImpactEffects.h"
#include "Sound/SoundBase.h"

namespace
{
	const FLinearColor RecordingRed(0.9f, 0.05f, 0.05f);
	const FLinearColor Dimmed(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor HighlightGold(1.f, 0.78f, 0.2f);
	// How wide a menu's rows are, on a screen 1920 wide.
	const float MenuRowWidth = 900.f;

	// What a scale of 1 is in points of the outline font: chosen so text is about the size the bitmap font drew it.
	const float PointsPerScale = 10.5f;

	FSlateFontInfo SharpFont(float Scale)
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(FMath::RoundToInt32(Scale * PointsPerScale), 6));
	}

	// The director watches the script being ignored, and says so: the next of the scene's lines after every so many blows.
	const int32 HitsBetweenShouts = 6;
	const float KeysStaySeconds = 14.f;
	const float ShoutSeconds = 2.8f;
}

void AStickHUD::BeginPlay()
{
	Super::BeginPlay();

	OutlineFontCarrier = NewObject<UFont>(this);
	OutlineFontCarrier->FontCacheType = EFontCacheType::Runtime;

	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnTakeStarted.AddDynamic(this, &AStickHUD::HandleTakeStarted);
	Events->OnCut.AddDynamic(this, &AStickHUD::HandleCut);
	Events->OnTakeFinished.AddDynamic(this, &AStickHUD::HandleTakeFinished);
	Events->OnFinalCutStarted.AddDynamic(this, &AStickHUD::HandleFinalCutStarted);
	Events->OnCard.AddDynamic(this, &AStickHUD::HandleCard);
	Events->OnPayChanged.AddDynamic(this, &AStickHUD::HandlePayChanged);
	Events->OnAction.AddDynamic(this, &AStickHUD::HandleAction);
	Events->OnWardrobe.AddDynamic(this, &AStickHUD::HandleWardrobe);
	Events->OnBoxOffice.AddDynamic(this, &AStickHUD::HandleBoxOffice);
	Events->OnHit.AddDynamic(this, &AStickHUD::HandleHit);
}

void AStickHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	const float Width = Canvas->SizeX;
	const float Height = Canvas->SizeY;
	// Everything is laid out for a 1920 by 1080 screen and scaled from there. In a window narrower than that shape the
	// layout shrinks to the window's width and sits in the middle of its height, so no line runs off the side.
	const float Unit = FMath::Min(Height / 1080.f, Width / 1920.f);
	const float Top = (Height - 1080.f * Unit) * 0.5f;
	const float Now = GetWorld()->GetTimeSeconds();
	UFont* Font = GEngine->GetLargeFont();

	// The pointer shows over a card or the wardrobe, where it chooses. Anywhere else the game has the mouse to itself.
	const bool bPointerWanted = Slate == ESlate::Card || Slate == ESlate::Wardrobe;
	if (PlayerOwner && bPointerWanted != PlayerOwner->bShowMouseCursor)
	{
		PlayerOwner->bShowMouseCursor = bPointerWanted;
		if (bPointerWanted)
		{
			FInputModeGameAndUI Mode;
			Mode.SetHideCursorDuringCapture(false);
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerOwner->SetInputMode(Mode);
		}
		else
		{
			PlayerOwner->SetInputMode(FInputModeGameOnly());
		}
	}
	float PointerX = -1.f;
	float PointerY = -1.f;
	Pointer = bPointerWanted && PlayerOwner && PlayerOwner->GetMousePosition(PointerX, PointerY) ? FVector2D(PointerX, PointerY - Top) : FVector2D(-1., -1.);
	bPointerMoved = !Pointer.Equals(LastPointer, 0.5);
	LastPointer = Pointer;
	if (!Flow.IsValid())
	{
		Flow = Cast<AGameFlow>(UGameplayStatics::GetActorOfClass(this, AGameFlow::StaticClass()));
	}

	if (Slate == ESlate::Film)
	{
		const AFinalCut* Film = FilmShown.Get();
		if (Film && Film->IsOnTitleCard())
		{
			// Black, as a film's own title card is. The band is already playing.
			DrawRect(FLinearColor::Black, 0.f, 0.f, Width, Height);
			DrawCentered(TEXT("A  PAGE  TWELVE  PICTURE"), Top + 320.f * Unit, 1.6f * Unit, FLinearColor(0.6f, 0.6f, 0.6f));
			DrawCentered(FilmTitle, Top + 410.f * Unit, 6.5f * Unit, FLinearColor::White);
			DrawRect(RecordingRed, Width * 0.5f - 90.f * Unit, Top + 572.f * Unit, 180.f * Unit, 6.f * Unit);
			DrawCentered(TEXT("THE DIRECTOR'S CUT"), Top + 612.f * Unit, 3.f * Unit, HighlightGold);
			DrawCentered(FString::Printf(TEXT("%s      TAKE %d"), *FilmCut, CurrentTake), Top + 700.f * Unit, 1.9f * Unit, FLinearColor(0.75f, 0.75f, 0.75f));
			return;
		}

		// The film is letterboxed and carries no game interface: it is the result, not the shoot.
		const float Bar = Height * 0.11f;
		DrawRect(FLinearColor::Black, 0.f, 0.f, Width, Bar);
		DrawRect(FLinearColor::Black, 0.f, Height - Bar, Width, Bar);
		Write(FString::Printf(TEXT("DIRECTOR'S CUT: %s   TAKE %d"), *FilmCut, CurrentTake), FLinearColor::White, 48.f * Unit, Bar * 0.3f, Font, 2.f * Unit);

		const FString Skip = TEXT("Punch to skip");
		float SkipWidth = 0.f;
		float SkipHeight = 0.f;
		Measure(Skip, SkipWidth, SkipHeight, Font, 1.4f * Unit);
		Write(Skip, FLinearColor(0.55f, 0.55f, 0.55f), Width - SkipWidth - 48.f * Unit, Height - Bar * 0.65f, Font, 1.4f * Unit);

		if (Film && Film->IsOnEndSlate())
		{
			// A clapperboard over the last frame: its striped stick, then what a slate says.
			const float BoardWidth = 860.f * Unit;
			const float Left = (Width - BoardWidth) * 0.5f;
			const float BoardTop = Top + 350.f * Unit;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.84f), Left, BoardTop, BoardWidth, 380.f * Unit);
			const int32 Stripes = 12;
			for (int32 Stripe = 0; Stripe < Stripes; Stripe += 2)
			{
				DrawRect(FLinearColor::White, Left + Stripe * BoardWidth / Stripes, BoardTop, BoardWidth / Stripes, 36.f * Unit);
			}
			const int32 Ran = FMath::RoundToInt32(Film->GetRunningSeconds());
			DrawCentered(TEXT("STICK TO THE SCRIPT"), BoardTop + 62.f * Unit, 2.f * Unit, FLinearColor(0.75f, 0.75f, 0.75f));
			DrawCentered(FString::Printf(TEXT("%s      TAKE %d"), *FilmScene, CurrentTake), BoardTop + 118.f * Unit, 4.2f * Unit, FLinearColor::White);
			DrawCentered(FString::Printf(TEXT("%s      %d:%02d"), *FilmCut, Ran / 60, Ran % 60), BoardTop + 222.f * Unit, 2.2f * Unit, HighlightGold);
			DrawCentered(TEXT("CUT.  PRINT IT."), BoardTop + 292.f * Unit, 2.6f * Unit, FLinearColor::White);
		}
		return;
	}

	// The film's bars hug the window's own edges. Everything else is drawn inside the middle band.
	struct FMovedDown
	{
		FMovedDown(FCanvas* InCanvas, float By) : Moved(InCanvas) { Moved->PushRelativeTransform(FTranslationMatrix(FVector(0., By, 0.))); }
		~FMovedDown() { Moved->PopTransform(); }
		FCanvas* Moved;
	} MovedDown(Canvas->Canvas, Top);

	if (Slate == ESlate::Wardrobe)
	{
		// No dark card here: the point is to see the fighter change.
		DrawCentered(TEXT("WARDROBE"), 96.f * Unit, 5.f * Unit, FLinearColor::White);
		DrawRect(RecordingRed, Width * 0.5f - 90.f * Unit, 212.f * Unit, 180.f * Unit, 6.f * Unit);
		DrawWardrobe(Unit);
		return;
	}

	if (Slate == ESlate::Card)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.66f), 0.f, -Top, Width, Height);
		DrawCentered(CardHeadline, 270.f * Unit, 6.5f * Unit, FLinearColor::White);
		DrawRect(RecordingRed, Width * 0.5f - 90.f * Unit, 405.f * Unit, 180.f * Unit, 6.f * Unit);
		// A long card closes its lines up, so the last one stays on the screen.
		const float LineStep = FMath::Min(62.f, 560.f / FMath::Max(CardLines.Num() - 1, 1));
		const float LineSize = 2.4f * FMath::Sqrt(LineStep / 62.f);
		int32 MenuRow = 0;
		for (int32 Line = 0; Line < CardLines.Num(); ++Line)
		{
			const FString& Text = CardLines[Line];
			const float Y = (470.f + Line * LineStep) * Unit;
			const bool bChosen = Text.StartsWith(TEXT(">\t"));
			if (bChosen || Text.StartsWith(TEXT("\t")))
			{
				DrawMenuRow(Text.RightChop(bChosen ? 2 : 1), bChosen, Y, LineStep * Unit, LineSize * Unit, Unit);
				// The row the pointer has just moved onto becomes the chosen one. A pointer lying still leaves the keys alone.
				if (bPointerMoved && Flow.IsValid() && IsPointerIn((Width - MenuRowWidth * Unit) * 0.5f, Y - 4.f * Unit, MenuRowWidth * Unit, LineStep * Unit))
				{
					Flow->PointAtRow(MenuRow);
				}
				++MenuRow;
			}
			else
			{
				DrawCentered(Text, Y, LineSize * Unit, Text.StartsWith(TEXT("In the bank")) ? HighlightGold : FLinearColor::White);
			}
		}
		return;
	}

	const bool bRecording = Slate == ESlate::GetReady || Slate == ESlate::None;
	if (bRecording && CurrentTake > 0)
	{
		if (FMath::Fmod(Now, 1.f) < 0.6f)
		{
			DrawRect(RecordingRed, 48.f * Unit, 50.f * Unit, 22.f * Unit, 22.f * Unit);
		}
		Write(FString::Printf(TEXT("REC   TAKE %d"), CurrentTake), FLinearColor::White, 84.f * Unit, 44.f * Unit, Font, 2.f * Unit);

		if (CurrentPay != INDEX_NONE)
		{
			// What this scene will pay if the take in progress is the one.
			const FString Pay = FString::Printf(TEXT("$%d"), CurrentPay);
			float PayWidth = 0.f;
			float PayHeight = 0.f;
			Measure(Pay, PayWidth, PayHeight, Font, 2.4f * Unit);
			Write(Pay, FLinearColor::White, Width - PayWidth - 48.f * Unit, 40.f * Unit, Font, 2.4f * Unit);

			if (Now < EarnedUntilSeconds)
			{
				Measure(EarnedLine, PayWidth, PayHeight, Font, 1.6f * Unit);
				Write(EarnedLine, bEarnedSomething ? FLinearColor::White : FLinearColor(0.55f, 0.55f, 0.55f), Width - PayWidth - 48.f * Unit, 96.f * Unit, Font, 1.6f * Unit);
			}
		}

		if (const AStickFighter* Player = FindPlayerFighter())
		{
			const float Health = Player->GetHealthFraction();
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), 48.f * Unit, 98.f * Unit, 320.f * Unit, 18.f * Unit);
			DrawRect(Health < 0.3f ? RecordingRed : FLinearColor::White, 51.f * Unit, 101.f * Unit, 314.f * Unit * Health, 12.f * Unit);
			if (const AThrownProp* Held = Player->GetCarried(); Held && Held->IsGun())
			{
				// What is in the hands, and how many rounds are left in it. Empty, it is red: all it is good for is throwing.
				Write(FString::Printf(TEXT("%s   %d"), Held->GetGunName(), Held->GetShotsLeft()), Held->GetShotsLeft() > 0 ? HighlightGold : RecordingRed, 48.f * Unit, 130.f * Unit, Font, 2.f * Unit);
			}

			if (Slate == ESlate::None && Now < ShoutUntilSeconds)
			{
				DrawCentered(Shout, 960.f * Unit, 2.f * Unit, FLinearColor::White);
			}
			// What the strip below cannot say by itself is told as every take is called, and a while into a scene's first
			// one, unless the director is talking over it.
			else if (Slate == ESlate::GetReady || (Slate == ESlate::None && Now < KeysUntilSeconds))
			{
				DrawCentered(TEXT("Hold guard or duck and strike for other moves     Jump under a red bar to hang from it, then kick     Esc pause"), 966.f * Unit, 1.4f * Unit, FLinearColor(0.78f, 0.78f, 0.78f));
			}
			DrawKeys(*Player, Unit);
		}
	}

	switch (Slate)
	{
	case ESlate::GetReady:
		DrawCentered(FString::Printf(TEXT("TAKE %d"), CurrentTake), 380.f * Unit, 6.f * Unit, FLinearColor::White);
		DrawCentered(TEXT("...and action!"), 520.f * Unit, 2.5f * Unit, FLinearColor::White);
		if (const AStickFighter* Player = FindPlayerFighter())
		{
			if (Player->DamageTakenScale < 1.f)
			{
				DrawCentered(TEXT("The stunt crew is getting tired. They hit softer now."), 610.f * Unit, 2.f * Unit, HighlightGold);
			}
		}
		break;

	case ESlate::Cut:
		DrawRect(Dimmed, 0.f, -Top, Width, Height);
		DrawCentered(TEXT("CUT!"), 330.f * Unit, 10.f * Unit, RecordingRed);
		DrawCentered(TEXT("You went down, exactly as the script said. Dull."), 560.f * Unit, 3.f * Unit, FLinearColor::White);
		DrawCentered(TEXT("Back to one. Going again."), 650.f * Unit, 2.f * Unit, FLinearColor::White);
		break;

	case ESlate::Wrap:
		DrawCentered(TEXT("THAT'S A WRAP"), 380.f * Unit, 6.f * Unit, FLinearColor::White);
		DrawCentered(FString::Printf(TEXT("Take %d is the one"), CurrentTake), 520.f * Unit, 2.5f * Unit, FLinearColor::White);
		break;

	default:
		break;
	}

}

void AStickHUD::DrawKeys(const AStickFighter& Player, float Unit)
{
	auto Down = [this](std::initializer_list<FKey> Keys)
	{
		for (const FKey& Key : Keys)
		{
			if (PlayerOwner && PlayerOwner->IsInputKeyDown(Key))
			{
				return true;
			}
		}
		return false;
	};

	const AThrownProp* Held = Player.GetCarried();
	const bool bHanging = Player.IsHanging();
	// A strike with a throwable in the hands throws it. A gun and a weapon that is swung stay in them.
	const bool bThrows = Held && !Held->IsGun() && !Held->IsSwung();
	const bool bDucking = !Held && Player.IsDucking();
	const bool bGuarding = !Held && Player.IsGuarding();

	const TCHAR* Punch = bHanging ? TEXT("Swing kick") : bThrows ? TEXT("Throw") : Held && Held->IsGun() ? (Held->GetShotsLeft() > 0 ? TEXT("Fire") : TEXT("Empty"))
		: Held ? TEXT("Swing") : bDucking ? TEXT("Uppercut") : bGuarding ? TEXT("Hook") : TEXT("Punch");
	const TCHAR* Kick = bHanging ? TEXT("Swing kick") : bThrows ? TEXT("Throw") : bDucking ? TEXT("Sweep") : bGuarding ? TEXT("Roundhouse")
		: Player.IsAirborne() ? TEXT("Flying kick") : TEXT("Kick");
	const TCHAR* Grab = Held ? TEXT("Throw") : Player.FindPropInReach() ? TEXT("Pick up") : TEXT("Grab");

	struct FKeyChip
	{
		const TCHAR* Key;
		const TCHAR* Does;
		// What it does as a rule. Anything else is worth a look.
		const TCHAR* Plain;
		bool bDown;
	};
	// shortcut: the keys are written here by hand, they should be read from the input bindings if those ever change.
	const FKeyChip Chips[] = {
		{ TEXT("W A S D"), TEXT("Move"), TEXT("Move"), Down({ EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::Up, EKeys::Left, EKeys::Down, EKeys::Right }) },
		{ TEXT("Left click"), Punch, TEXT("Punch"), Down({ EKeys::LeftMouseButton, EKeys::J }) },
		{ TEXT("F"), Kick, TEXT("Kick"), Down({ EKeys::F, EKeys::K }) },
		{ TEXT("Right click"), TEXT("Guard"), TEXT("Guard"), Down({ EKeys::RightMouseButton, EKeys::LeftShift, EKeys::RightShift, EKeys::L }) },
		{ TEXT("Ctrl"), TEXT("Duck"), TEXT("Duck"), Down({ EKeys::LeftControl, EKeys::RightControl, EKeys::C }) },
		{ TEXT("Space"), bHanging ? TEXT("Let go") : TEXT("Jump"), TEXT("Jump"), Down({ EKeys::SpaceBar }) },
		{ TEXT("E"), Grab, TEXT("Grab"), Down({ EKeys::E }) },
	};

	const float Scale = 1.45f * Unit;
	const float Pad = 9.f * Unit;
	const float AfterKey = 9.f * Unit;
	const float BetweenChips = 30.f * Unit;
	float Widths[UE_ARRAY_COUNT(Chips)][2];
	float LineHeight = 0.f;
	float Total = -BetweenChips;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Chips); ++Index)
	{
		Measure(Chips[Index].Key, Widths[Index][0], LineHeight, nullptr, Scale);
		Measure(Chips[Index].Does, Widths[Index][1], LineHeight, nullptr, Scale);
		Total += Widths[Index][0] + Pad * 2.f + AfterKey + Widths[Index][1] + BetweenChips;
	}

	float X = (Canvas->SizeX - Total) * 0.5f;
	const float Y = 1018.f * Unit;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.4f), X - 22.f * Unit, Y - 10.f * Unit, Total + 44.f * Unit, LineHeight + 20.f * Unit);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Chips); ++Index)
	{
		const FKeyChip& Chip = Chips[Index];
		const bool bChanged = FCString::Strcmp(Chip.Does, Chip.Plain) != 0;
		DrawRect(Chip.bDown ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.16f), X, Y - 4.f * Unit, Widths[Index][0] + Pad * 2.f, LineHeight + 8.f * Unit);
		Write(Chip.Key, Chip.bDown ? FLinearColor::Black : FLinearColor::White, X + Pad, Y, nullptr, Scale);
		Write(Chip.Does, bChanged ? HighlightGold : FLinearColor(0.8f, 0.8f, 0.8f), X + Widths[Index][0] + Pad * 2.f + AfterKey, Y, nullptr, Scale);
		X += Widths[Index][0] + Pad * 2.f + AfterKey + Widths[Index][1] + BetweenChips;
	}
}

void AStickHUD::HandleBoxOffice(const FText& What, int32 Dollars)
{
	EarnedLine = FString::Printf(TEXT("+$%d  %s"), Dollars, *What.ToString());
	bEarnedSomething = Dollars > 0;
	EarnedUntilSeconds = GetWorld()->GetTimeSeconds() + 1.6f;
}

void AStickHUD::HandleWardrobe(const FText& Choices)
{
	if (Choices.IsEmpty())
	{
		if (Slate == ESlate::Wardrobe)
		{
			Slate = ESlate::None;
		}
		return;
	}

	Choices.ToString().ParseIntoArrayLines(WardrobeLines, false);
	Slate = ESlate::Wardrobe;
}

void AStickHUD::HandleAction()
{
	if (Slate == ESlate::GetReady)
	{
		Slate = ESlate::None;
	}
}

void AStickHUD::HandleHit(AActor* Attacker, AActor* Victim, EMove Move)
{
	const AStickFighter* Striker = Cast<AStickFighter>(Attacker);
	if (!Striker || Striker->Team != 0 || !Striker->FightsFreely() || Slate == ESlate::Film)
	{
		return;
	}

	if (PlayerHits++ % HitsBetweenShouts == 0 && !DirectorLines.IsEmpty())
	{
		const int32 Line = NextShout++ % DirectorLines.Num();
		Shout = TEXT("DIRECTOR: ") + DirectorLines[Line];
		ShoutUntilSeconds = GetWorld()->GetTimeSeconds() + ShoutSeconds;
		if (USoundBase* Voice = DirectorVoices[Line])
		{
			UGameplayStatics::PlaySound2D(this, Voice);
		}
	}
}

// The player's fighter is the one on the player's side that fights. A captive is on that side too, and does not.
const AStickFighter* AStickHUD::FindPlayerFighter()
{
	if (!PlayerFighter.IsValid())
	{
		for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
		{
			if (It->Team == 0 && It->FightsFreely())
			{
				PlayerFighter = *It;
				break;
			}
		}
	}
	return PlayerFighter.Get();
}

// The scene's own lines, and the director's voice for each where there is a recording. Asked for at the first take,
// because the scene is not yet known when the interface starts.
void AStickHUD::LearnDirectorLines()
{
	const ATakeDirector* Director = Cast<ATakeDirector>(UGameplayStatics::GetActorOfClass(this, ATakeDirector::StaticClass()));
	if (!Director)
	{
		return;
	}

	const UStickGameInstance* Game = Cast<UStickGameInstance>(GetGameInstance());
	DirectorLines = Director->Script.DirectorLines;
	for (int32 Line = 0; Line < DirectorLines.Num(); ++Line)
	{
		DirectorVoices.Add(AImpactEffects::FindRecording(FString::Printf(TEXT("V_Scene%d_%d"), Game ? Game->SceneIndex + 1 : 1, Line + 1)));
	}
}

void AStickHUD::HandleTakeStarted(int32 TakeNumber)
{
	if (DirectorLines.IsEmpty())
	{
		LearnDirectorLines();
	}
	PlayerHits = 0;
	ShoutUntilSeconds = 0.f;
	EarnedUntilSeconds = 0.f;
	CurrentTake = TakeNumber;
	KeysUntilSeconds = TakeNumber == 1 ? GetWorld()->GetTimeSeconds() + KeysStaySeconds : 0.f;
	Slate = ESlate::GetReady;
}

void AStickHUD::HandleCut(ECutReason Reason)
{
	Slate = ESlate::Cut;
}

void AStickHUD::HandleTakeFinished(const FTake& Take)
{
	Slate = ESlate::Wrap;
}

void AStickHUD::HandleFinalCutStarted(const FTake& Take)
{
	FilmShown = Cast<AFinalCut>(UGameplayStatics::GetActorOfClass(this, AFinalCut::StaticClass()));
	if (FilmShown.IsValid())
	{
		FilmCut = FString(AFinalCut::CutName(FilmShown->GetCut())).ToUpper();
	}
	if (const ATakeDirector* Director = Cast<ATakeDirector>(UGameplayStatics::GetActorOfClass(this, ATakeDirector::StaticClass())))
	{
		// "Scene 1: The warehouse" is "SCENE 1" on the slate and "THE WAREHOUSE" on the title card.
		const FString Name = Director->Script.Name.ToString().ToUpper();
		if (!Name.Split(TEXT(": "), &FilmScene, &FilmTitle))
		{
			FilmScene = Name;
			FilmTitle = Name;
		}
	}
	Slate = ESlate::Film;
}

void AStickHUD::HandleCard(const FText& Headline, const FText& Detail)
{
	if (Headline.IsEmpty())
	{
		if (Slate == ESlate::Card)
		{
			Slate = SlateUnderCard;
		}
		return;
	}

	if (Slate != ESlate::Card)
	{
		SlateUnderCard = Slate;
	}
	CardHeadline = Headline.ToString();
	Detail.ToString().ParseIntoArrayLines(CardLines, false);
	Slate = ESlate::Card;
}

void AStickHUD::HandlePayChanged(int32 Pay)
{
	CurrentPay = Pay;
}

void AStickHUD::DrawMenuRow(const FString& Text, bool bChosen, float Y, float Height, float Scale, float Unit)
{
	UFont* Font = GEngine->GetLargeFont();
	const float BarWidth = MenuRowWidth * Unit;
	const float BarHeight = Height - 8.f * Unit;
	const float X = (Canvas->SizeX - BarWidth) * 0.5f;
	DrawRect(bChosen ? HighlightGold : FLinearColor(1.f, 1.f, 1.f, 0.07f), X, Y - 4.f * Unit, BarWidth, BarHeight);

	// The shop writes a row as a name, a run of spaces, and what it costs or that it is owned.
	FString Name = Text;
	FString State;
	Text.Split(TEXT("     "), &Name, &State);
	Name.TrimStartAndEndInline();
	State.TrimStartAndEndInline();

	const FLinearColor Ink = bChosen ? FLinearColor::Black : FLinearColor::White;
	float NameWidth = 0.f;
	float TextHeight = 0.f;
	Measure(Name, NameWidth, TextHeight, Font, Scale);
	const float TextY = Y - 4.f * Unit + (BarHeight - TextHeight) * 0.5f;
	if (State.IsEmpty())
	{
		Write(Name, Ink, (Canvas->SizeX - NameWidth) * 0.5f, TextY, Font, Scale);
		return;
	}
	float StateWidth = 0.f;
	Measure(State, StateWidth, TextHeight, Font, Scale);
	Write(Name, Ink, X + 28.f * Unit, TextY, Font, Scale);
	Write(State, Ink, X + BarWidth - 28.f * Unit - StateWidth, TextY, Font, Scale);
}

bool AStickHUD::IsPointerIn(float X, float Y, float Width, float Height) const
{
	return Pointer.X >= X && Pointer.X <= X + Width && Pointer.Y >= Y && Pointer.Y <= Y + Height;
}

void AStickHUD::DrawWardrobe(float Unit)
{
	const UStickGameInstance* Game = GetGameInstance<UStickGameInstance>();
	if (!Game)
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();
	// To the right of the fighter, who stands in the middle of the picture.
	const float Left = Canvas->SizeX * 0.5f + 290.f * Unit;
	const float PanelWidth = 590.f * Unit;
	const float Margin = 34.f * Unit;
	float Y = 300.f * Unit;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f), Left - Margin, Y - Margin, PanelWidth + Margin * 2.f, 566.f * Unit);
	DrawRect(HighlightGold, Left - Margin, Y - Margin, PanelWidth + Margin * 2.f, 5.f * Unit);

	const int32 Chosen[] = { Game->Look.Color, Game->Look.Hat, Game->Look.Build };
	const TCHAR* Titles[] = { TEXT("COLOUR"), TEXT("HAT"), TEXT("BUILD") };
	const TCHAR* Keys[] = { TEXT("A   D"), TEXT("W   S"), TEXT("F") };
	const FLinearColor Quiet(0.62f, 0.62f, 0.62f);
	int32 PointedRow = INDEX_NONE;
	int32 PointedIndex = INDEX_NONE;
	for (int32 Row = 0; Row < 3; ++Row)
	{
		float KeyWidth = 0.f;
		float KeyHeight = 0.f;
		Measure(Keys[Row], KeyWidth, KeyHeight, Font, 1.5f * Unit);
		Write(Titles[Row], Quiet, Left, Y, Font, 1.5f * Unit);
		Write(Keys[Row], HighlightGold, Left + PanelWidth - KeyWidth, Y, Font, 1.5f * Unit);
		Y += 46.f * Unit;

		const int32 Count = AStickFighter::LookCount(Row);
		const float Gap = 9.f * Unit;
		const float Each = (PanelWidth - Gap * (Count - 1)) / Count;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float X = Left + Index * (Each + Gap);
			// What has to be bought in the shop first is shown faint: it is there, and not yet yours.
			const bool bOwned = Game->OwnsLook(Row, Index);
			// A pip is thin, so the pointer is given some room above and below it.
			const bool bPointed = bOwned && (Row == 0 ? IsPointerIn(X, Y, Each, Each) : IsPointerIn(X, Y - 14.f * Unit, Each, 37.f * Unit));
			if (bPointed)
			{
				PointedRow = Row;
				PointedIndex = Index;
				DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.35f), X - 3.f * Unit, Y - 3.f * Unit, Each + 6.f * Unit, (Row == 0 ? Each : 9.f * Unit) + 6.f * Unit);
			}
			if (Row == 0)
			{
				if (Index == Chosen[Row])
				{
					DrawRect(FLinearColor::White, X - 4.f * Unit, Y - 4.f * Unit, Each + 8.f * Unit, Each + 8.f * Unit);
				}
				FLinearColor Swatch = AStickFighter::LookColor(Index);
				Swatch.A = bOwned ? 1.f : 0.22f;
				DrawRect(Swatch, X, Y, Each, Each);
			}
			else
			{
				DrawRect(Index == Chosen[Row] ? HighlightGold : FLinearColor(1.f, 1.f, 1.f, bOwned ? 0.4f : 0.1f), X, Y, Each, 9.f * Unit);
			}
		}
		Y += (Row == 0 ? Each : 9.f * Unit) + 16.f * Unit;
		Write(AStickFighter::LookName(Row, Chosen[Row]), FLinearColor::White, Left, Y, Font, 2.8f * Unit);
		Y += 96.f * Unit;
	}

	if (Flow.IsValid())
	{
		Flow->PointAtLook(PointedRow, PointedIndex);
	}
	DrawCentered(TEXT("Click a colour, a hat or a build to wear it.   Click anywhere else when you are ready."), 990.f * Unit, 1.9f * Unit, FLinearColor::White);
}

void AStickHUD::DrawCentered(const FString& Text, float Y, float Scale, const FLinearColor& Color)
{
	UFont* Font = GEngine->GetLargeFont();
	float TextWidth = 0.f;
	float TextHeight = 0.f;
	Measure(Text, TextWidth, TextHeight, Font, Scale);

	const float X = (Canvas->SizeX - TextWidth) * 0.5f;
	Write(Text, FLinearColor::Black, X + 2.f, Y + 2.f, Font, Scale);
	Write(Text, Color, X, Y, Font, Scale);
}

void AStickHUD::Write(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Unused, float Scale)
{
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), SharpFont(Scale), Color);
	Item.Font = OutlineFontCarrier;
	Canvas->DrawItem(Item);
}

void AStickHUD::Measure(const FString& Text, float& OutWidth, float& OutHeight, UFont* Unused, float Scale) const
{
	const FVector2D Size(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, SharpFont(Scale)));
	OutWidth = static_cast<float>(Size.X);
	OutHeight = static_cast<float>(Size.Y);
}
