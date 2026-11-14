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
	// The wardrobe's panel: every colour as a swatch, every hat and build as a pip, what is chosen named large.
	void DrawWardrobe(float Unit);

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
	// The keys stay on screen until then: some seconds into a scene's first take.
	float KeysUntilSeconds = 0.f;
	TArray<FString> WardrobeLines;
	FString EarnedLine;
	bool bEarnedSomething = false;
	float EarnedUntilSeconds = 0.f;
};
