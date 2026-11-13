#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "GameFlow.generated.h"

class AStickFighter;
class ATakeDirector;

// What the audience pays for. A film is worth more the more it shows, and nothing for the same move over and over.
struct FBoxOffice
{
	// What a blow the player landed earns. A move seen more than three times running earns nothing until another is used.
	// A knockout always pays.
	int32 Blow(EMove Move, bool bFromTheAir, bool bKnockout);

	// What stopping a blow on a raised guard earns.
	int32 Guard();

	// Paid once, at the end: so much for every kind of move beyond the first.
	int32 Variety() const;
	int32 Total() const { return Earned + Variety(); }
	FString Describe() const;

	int32 Earned = 0;
	int32 Knockouts = 0;
	int32 Thrown = 0;
	int32 FromTheAir = 0;
	int32 Guards = 0;

private:
	uint16 KindsSeen = 0;
	uint8 LastKind = MAX_uint8;
	int32 SameInARow = 0;
};

// The run of the whole game: title, then each scene (its card, the shoot, the film, the pay), then the results.
UCLASS()
class AGameFlow : public AActor
{
	GENERATED_BODY()

public:
	AGameFlow();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Every move the player makes outside a fight comes through here: it answers a card or a menu, or skips the film.
	void SubmitMove(EMove Move);

	// Any key that is not a move: it only ever answers a card that waits for one.
	void AnswerCard();

	// While the wardrobe is open: moves one choice on or back. Row 0 is the colour, 1 the hat, 2 the build.
	// On a menu, the hat's keys walk the cursor.
	void ChangeLook(int32 Row, int32 Direction);

	// Stops a take or its film with a short menu over it, or takes that menu away again.
	void TogglePause();

	// The share of a blow's damage the player takes in a given take. The first few takes are fought at full strength.
	// After that the opponents tire a little more with every retake, so that nobody is stuck in a scene for good.
	static float DamageShareInTake(int32 TakeNumber);

private:
	enum class EStep : uint8
	{
		Title,
		Wardrobe,
		SceneCard,
		Shooting,
		// The wrap is on screen for a moment, then the player picks how the film is cut.
		Wrapped,
		Cutting,
		Film,
		Paid,
		Results,
		Credits,
		Shop
	};

	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void HandleTakeFinished(const FTake& Take);

	UFUNCTION()
	void HandleFinalCutFinished();

	UFUNCTION()
	void HandleHit(AActor* Attacker, AActor* Victim, EMove Move);

	UFUNCTION()
	void HandleBlocked(AActor* Attacker, AActor* Defender);

	void Announce(const TCHAR* What, int32 Dollars);
	void SkipFilm();
	void ShowWardrobe();
	AStickFighter* FindPlayerFighter() const;

	bool WaitsForKey() const;
	void ShowMenu();
	void ChooseFromMenu();
	void BackToMenu();
	bool IsUnlocked(int32 Scene) const;
	FString BringBoughtWeapon();

	void Enter(EStep NewStep, float Seconds = 0.f);
	void Continue();
	void ShowCard(const FString& Headline, const FString& Detail);
	void OpenStage(const FString& StagePackage);
	int32 PayAfter(int32 Takes) const;

	UPROPERTY()
	TObjectPtr<ATakeDirector> Director;

	EStep Step = EStep::Title;
	float StepSeconds = 0.f;
	float StepSecondsLeft = 0.f;
	int32 TakesUsed = 1;
	int32 CurrentTake = 1;
	// Counted afresh in every take: only the take that is the one gets to the cinema.
	FBoxOffice BoxOffice;

	// The cursor walks the title's rows, or the pause menu's while the game is paused.
	TArray<FString> SceneNames;
	int32 MenuCursor = 0;
	int32 MenuRows = 0;
	bool bPaused = false;
	// What the shop last had to say: sold, or why not.
	FString ShopNote;
};
