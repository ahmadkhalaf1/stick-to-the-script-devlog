#pragma once

#include "CoreMinimal.h"
#include "TakeTypes.generated.h"

// Stored by value in scene scripts and recorded takes: append new moves, never reorder.
UENUM(BlueprintType)
enum class EMove : uint8
{
	None,
	Block,
	Duck,
	Punch,
	Kick,
	Throw,
	Shot,
	// Strikes posed in code, each started from a held pose: an uppercut and a sweep from a duck, a chop and a
	// roundhouse from a raised guard, and a kick swung from a bar overhead.
	Uppercut,
	Sweep,
	Roundhouse,
	Chop,
	SwingKick,
	// The blaster going off: a spread of pellets. A pistol, a rifle or the gunman's gun is Shot.
	Blast
};

// A blow with the weight of the body behind it. It staggers further, and looks and sounds heavier.
inline bool IsHeavyMove(EMove Move)
{
	return Move != EMove::Punch && Move != EMove::Chop && Move != EMove::Sweep;
}

// Why a take was cut. Going down is the only way there is.
UENUM(BlueprintType)
enum class ECutReason : uint8
{
	KnockedOut
};

// How an opponent fights. Stored by name in scene files: append, never reorder.
UENUM(BlueprintType)
enum class EOpponentKind : uint8
{
	Thug,
	Rusher,
	Kicker,
	Heavy,
	Gunman
};

// One opponent of a scene.
USTRUCT(BlueprintType)
struct FOpponent
{
	GENERATED_BODY()

	// Where it waits out of sight until it joins the fight: behind cover, or beyond the edge of the picture.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Enter = FVector2D::ZeroVector;

	// Where it comes to when it joins the fight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Spot = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color = FLinearColor(0.02f, 0.02f, 0.02f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOpponentKind Kind = EOpponentKind::Thug;

	// It joins when the player comes this close to its spot, or when fewer than two others are still fighting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WakeDistance = 520.f;
};

UENUM(BlueprintType)
enum class EPropKind : uint8
{
	Crate,
	Bottle,
	Melon,
	Cone,
	Pan,
	// Legendary: it is not on the set. It falls where an opponent was knocked out.
	Hammer,
	Bat,
	Fish,
	// Bought in the shop for one scene. Held and swung like the golden ones, without the knockout in one blow.
	WoodBat,
	IronBar,
	Sledge,
	// Guns: the punch fires one while it has rounds left, and after that it is thrown like anything else.
	Pistol,
	Blaster,
	Rifle
};

// Something loose on the set that a fighter can pick up and throw.
USTRUCT(BlueprintType)
struct FSceneProp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Location = FVector2D::ZeroVector;

	// How far above the floor it rests: 0 on the floor, more on a bar or a shelf.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPropKind Kind = EPropKind::Crate;
};

// A pipe overhead, across the set, to hang from and kick off.
USTRUCT(BlueprintType)
struct FSceneBar
{
	GENERATED_BODY()

	// Where along the set it crosses.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float X = 0.f;

	// How far above the floor. A jump reaches a little over 3 m.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height = 250.f;

	// Out of doors there is no ceiling to hang it from, so it stands on a post behind the fight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOnAPost = false;
};

USTRUCT(BlueprintType)
struct FSceneScript
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Name;

	// What the screenplay says happens. The player does something else.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Synopsis;

	// What the director shouts as the fight goes against the script, in the order he shouts it. Each scene has its own.
	// A recording named V_Scene<number>_<line> in Content/Audio is his voice for a line; without one it is only written.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> DirectorLines;

	// The part of the floor the fighters may walk on.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D ArenaMin = FVector2D(-500., -150.);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D ArenaMax = FVector2D(500., 150.);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSceneProp> Props;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSceneBar> Bars;

	// Where someone waits to be rescued: outside the arena, cowering until every opponent is down.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector2D> Captives;

	// Shown on the last card of the run, when this is the last scene: how the film ends now.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Epilogue;

	// Where the player's fighter starts every take.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D PlayerStart = FVector2D::ZeroVector;

	// Empty means the level's tagged marks are used: up to three opponents, one on each.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FOpponent> Opponents;

	// The level the scene is shot on, as a package path such as /Game/Scenes/Stage02. Empty means the level already open.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Stage;

	// What a clean first take pays, and what every "Cut!" takes off it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Fee = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CutCost = 50;
};

UENUM(BlueprintType)
enum class ERecordedEventKind : uint8
{
	Hit,
	Blocked,
	Cut,
	// A gun going off, whether it hit anyone or not: the film wants its flash and its noise back.
	Shot
};

// One UTakeEvents event as it happened during a take. Fields the kind does not use keep their defaults.
USTRUCT(BlueprintType)
struct FRecordedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	ERecordedEventKind Kind = ERecordedEventKind::Hit;

	UPROPERTY(BlueprintReadWrite)
	float TimeSeconds = 0.f;

	UPROPERTY(BlueprintReadWrite)
	EMove Move = EMove::None;

	// Indices into FTakeFrame::Fighters.
	UPROPERTY(BlueprintReadWrite)
	int32 AttackerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	int32 VictimIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	ECutReason CutReason = ECutReason::KnockedOut;

	// Hit and Blocked: where on the victim's or defender's body the strike landed.
	UPROPERTY(BlueprintReadWrite)
	FVector ImpactPoint = FVector::ZeroVector;

	// Hit: the victim went down.
	UPROPERTY(BlueprintReadWrite)
	bool bKnockout = false;
};

USTRUCT(BlueprintType)
struct FFighterPose
{
	GENERATED_BODY()

	// World positions, in the order AStickFighter::GetJointPositions returns them.
	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> Joints;

	// Bit 0 is the left arm, bit 1 the right. A broken arm is drawn differently, and the joints alone do not show it.
	// Bit 2 says the fighter holds a gun, which the joints do not show either.
	UPROPERTY(BlueprintReadWrite)
	uint8 BrokenArms = 0;
};

USTRUCT(BlueprintType)
struct FTakeFrame
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float TimeSeconds = 0.f;

	// Same order in every frame: the player first, then the opponents in the scene's order, then any captive.
	UPROPERTY(BlueprintReadWrite)
	TArray<FFighterPose> Fighters;

	// The loose props, in the order the scene lists them.
	UPROPERTY(BlueprintReadWrite)
	TArray<FTransform> Props;
};

USTRUCT(BlueprintType)
struct FTake
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 Number = 0;

	UPROPERTY(BlueprintReadWrite)
	bool bClean = false;

	UPROPERTY(BlueprintReadWrite)
	TArray<FRecordedEvent> Events;

	UPROPERTY(BlueprintReadWrite)
	TArray<FTakeFrame> Frames;

	// One per fighter, in the order of FTakeFrame::Fighters, so a replay shows the characters as they were played.
	UPROPERTY(BlueprintReadWrite)
	TArray<FLinearColor> FighterColors;
};

// How the player chose to look. Each number picks from a list the fighter keeps: see AStickFighter::LookCount.
struct FCharacterLook
{
	int32 Color = 0;
	int32 Hat = 0;
	int32 Build = 1;
};
