#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "StickHUD.generated.h"

class AStickFighter;
class USoundBase;

// Knows about a take only what the UTakeEvents events say, so the interface cannot break the take loop.
UCLASS()
class AStickHUD : public AHUD
{
	GENERATED_BODY()

public:
	AStickHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	enum class ESlate : uint8
	{
		None,
		GetReady,
		Cut,
		Wrap,
		Film,
		Card,
		Wardrobe
	};

	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void HandleCut(ECutReason Reason);

	UFUNCTION()
	void HandleTakeFinished(const FTake& Take);

	UFUNCTION()
	void HandleFinalCutStarted(const FTake& Take);

	UFUNCTION()
	void HandleOuttake(int32 TakeNumber, bool bCut);

	UFUNCTION()
	void HandleCard(const FText& Headline, const FText& Detail);

	UFUNCTION()
	void HandlePayChanged(int32 Pay);

	UFUNCTION()
	void HandleAction();

	UFUNCTION()
	void HandleWardrobe(const FText& Choices);

	UFUNCTION()
	void HandleBoxOffice(const FText& What, int32 Dollars);

	UFUNCTION()
	void HandleHit(AActor* Attacker, AActor* Victim, EMove Move);

	const AStickFighter* FindPlayerFighter();

	void DrawCentered(const FString& Text, float Y, float Scale, const FLinearColor& Color);
	// One row of a menu: a bar across the middle, gold under the chosen one. A row with a price or a state has it on the right.
	void DrawMenuRow(const FString& Text, bool bChosen, float Y, float Height, float Scale, float Unit);
	// The first menu: the scenes as picture cards on a path, in the order they are played, and the other rows under them.
	void DrawSceneMenu(float Unit);
	// A picture of each scene for its card, and the same out of focus for a scene still locked. Made by Tools/build_ui.py.
	UPROPERTY()
	TArray<TObjectPtr<class UTexture2D>> SceneCards;
	UPROPERTY()
	TArray<TObjectPtr<class UTexture2D>> LockedSceneCards;
	// The film as a screen with a play button, so nobody takes it for the fight still going on: small on the card that
	// offers the cut, and opening out to the whole picture between the film's bars as the film begins. Grown is 0 for
	// the small screen and 1 for the whole picture; Down is added to its place when nothing has moved the canvas.
	void DrawFilmScreen(float Unit, float Down, float Grown);
	// What the small screen shows: the picture as it was a moment after the last opponent of this take went down,
	// read back from the screen before anything of the HUD is drawn on it. Until there is one, the scene's card.
	UPROPERTY(Transient)
	TObjectPtr<class UTexture2D> TakeStill;
	// When to read it, by the game's clock, or 0 when none is wanted.
	float TakeStillAtSeconds = 0.f;
	void KeepTakeStill();
	// A play button's triangle, pointing right, Size high.
	void DrawPlayMark(float X, float Y, float Size, const FLinearColor& Color);
	// The wardrobe's panel: every colour as a swatch, every hat and build as a pip, what is chosen named large.
	void DrawWardrobe(float Unit);
	// A strip along the bottom of a take: every key, and what it does at this moment. A meaning that has changed, because
	// of what the fighter holds or how it stands, is in gold, and a key that is down is lit.
	void DrawKeys(const AStickFighter& Player, float Unit);

	// Where the mouse pointer is, in the layout's own coordinates, or off it at (-1, -1). It works the menus and the
	// wardrobe; in a take it is hidden and the click is a punch.
	FVector2D Pointer = FVector2D(-1., -1.);
	FVector2D LastPointer = FVector2D(-1., -1.);
	bool bPointerMoved = false;
	bool IsPointerIn(float X, float Y, float Width, float Height) const;
	TWeakObjectPtr<class AGameFlow> Flow;

	// Text drawn from the engine's outline font at the size asked for. The bitmap font blurs when it is scaled up,
	// and the cards are mostly text. They take the same arguments as AHUD's DrawText and GetTextSize, font included.
	void Write(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Unused, float Scale);
	void Measure(const FString& Text, float& OutWidth, float& OutHeight, UFont* Unused, float Scale) const;
	void LearnDirectorLines();

	// The engine's canvas text will not draw without a font object, even when it is handed an outline font to use.
	// This empty one, marked as cached at run time, is what lets the outline font through.
	UPROPERTY()
	TObjectPtr<UFont> OutlineFontCarrier;

	ESlate Slate = ESlate::None;
	// What a card covered, shown again when the card goes: a pause can come at any moment of a take.
	ESlate SlateUnderCard = ESlate::None;
	int32 CurrentTake = 0;
	FString CardHeadline;
	FString FilmCut;
	// For the film's title card and the slate at its end, which the film says when to show.
	FString FilmScene;
	FString FilmTitle;
	TWeakObjectPtr<const class AFinalCut> FilmShown;
	// The take of the outtake on screen after the film, 0 during the film itself, and whether its "Cut!" has come.
	int32 OuttakeTake = 0;
	bool bOuttakeCut = false;
	TArray<FString> CardLines;
	int32 CurrentPay = INDEX_NONE;

	TWeakObjectPtr<AStickFighter> PlayerFighter;
	int32 PlayerHits = 0;
	int32 NextShout = 0;
	FString Shout;
	TArray<FString> DirectorLines;
	// The director's lines spoken, where Content/Audio has them. A line without a recording is only written.
	UPROPERTY()
	TArray<TObjectPtr<USoundBase>> DirectorVoices;
	float ShoutUntilSeconds = 0.f;
	bool bBossInTheFight = false;
	// The keys stay on screen until then: some seconds into a scene's first take.
	float KeysUntilSeconds = 0.f;
	TArray<FString> WardrobeLines;
	FString EarnedLine;
	bool bEarnedSomething = false;
	float EarnedUntilSeconds = 0.f;
};
