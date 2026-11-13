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
#include "StickToTheScript/Core/StickGameInstance.h"
#include "StickToTheScript/Core/TakeDirector.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "StickToTheScript/UI/ImpactEffects.h"
#include "Sound/SoundBase.h"

namespace
{
	const FLinearColor RecordingRed(0.9f, 0.05f, 0.05f);
	const FLinearColor Dimmed(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor HighlightGold(1.f, 0.78f, 0.2f);

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

	if (Slate == ESlate::Film)
	{
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
		for (int32 Line = 0; Line < WardrobeLines.Num(); ++Line)
		{
			DrawCentered(WardrobeLines[Line], (815.f + Line * 52.f) * Unit, 2.2f * Unit, FLinearColor::White);
		}
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
		for (int32 Line = 0; Line < CardLines.Num(); ++Line)
		{
			// A menu marks the row its cursor is on.
			DrawCentered(CardLines[Line], (470.f + Line * LineStep) * Unit, LineSize * Unit, CardLines[Line].StartsWith(TEXT(">")) ? HighlightGold : FLinearColor::White);
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

			if (Slate == ESlate::None && Now < ShoutUntilSeconds)
			{
				DrawCentered(Shout, 960.f * Unit, 2.f * Unit, FLinearColor::White);
			}
			// The keys show as every take is called, and stay a while into a scene's first one, unless the director is talking over them.
			else if (Slate == ESlate::GetReady || (Slate == ESlate::None && Now < KeysUntilSeconds))
			{
				// shortcut: the keys are written here by hand, S2-FEEL-02 should read them from the input bindings.
				DrawCentered(TEXT("W A S D move     Left click punch     Right click guard     F kick     Ctrl duck     Space jump     E grab, throw     Esc pause"), 950.f * Unit, 1.7f * Unit, FLinearColor::White);
				DrawCentered(TEXT("Duck and punch: uppercut     Duck and kick: sweep     Guard and punch: chop     Guard and kick: roundhouse     Jump under a red bar, then kick"), 1000.f * Unit, 1.4f * Unit, FLinearColor(0.75f, 0.75f, 0.75f));
			}
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
	if (const AFinalCut* Film = Cast<AFinalCut>(UGameplayStatics::GetActorOfClass(this, AFinalCut::StaticClass())))
	{
		FilmCut = FString(AFinalCut::CutName(Film->GetCut())).ToUpper();
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
