#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "ThrownProp.generated.h"

class AStickFighter;
class UMaterialInterface;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

// How a carrier holds a prop: a crate overhead in both hands, a bottle in one, a bat ready to swing, a pistol in two
// hands out in front, a rifle at the shoulder.
enum class EPropHold : uint8
{
	Overhead,
	OneHand,
	Swung,
	Pistol,
	LongGun
};

// Something loose on the set that a fighter can pick up and throw: a crate, a bottle.
UCLASS()
class AThrownProp : public AActor
{
	GENERATED_BODY()

public:
	AThrownProp();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Call once, where the prop rests: the actor's place is taken as the bottom of the prop.
	void SetUp(EPropKind InKind, double InFloorHeight, const FVector2D& InArenaMin, const FVector2D& InArenaMax);

	// A prop still waiting under the floor is not there to be picked up, however near it is on the plan.
	bool IsLoose() const { return State == EState::Resting && !bPuppet && !bInReserve && !bOffTheSet; }
	void PickUp(AStickFighter* NewHolder);

	// Let go with this velocity. With none it just falls, and hurts nobody.
	void Release(const FVector& InVelocity);

	// The final cut shows a recorded take by putting the prop where it was, frame by frame.
	void SetPuppetTransform(const FTransform& Transform);

	// A golden prop: whatever it hits is knocked out and sent flying.
	bool IsLegendary() const;

	// Held and swung with a punch, where anything else is thrown by one. E still throws it.
	bool IsSwung() const;
	float GetSwingDamage() const;

	// Keeps it out of sight until some knockout of the take, chosen afresh each take from the first to the one given,
	// then drops it where that opponent fell.
	void HoldInReserve(int32 LatestKnockoutToDropOn);

	// One blow landed with it. True when that was its last and it has broken.
	bool SpendUse();
	// A gun: the punch fires it while it has rounds left.
	bool IsGun() const;
	EPropHold GetHold() const;
	int32 GetShotsLeft() const { return RoundsLeft; }
	const TCHAR* GetGunName() const;
	// One pull of the trigger. False when it is empty or was fired a moment ago; otherwise what came out of it.
	bool TakeShot(int32& OutPellets, float& OutPelletDamage, float& OutSpreadDegrees, float& OutKnockSpeed, EMove& OutMove);
	// Kept out of sight until this fighter goes down, then dropped where he fell: a gunman's pistol.
	void HoldInReserveFor(AActor* Fighter);

	// The place kept in every scene for a weapon from the shop. It is made with the set and hidden, so the take
	// recorder has it from the first frame, and it comes on as whatever was bought.
	void KeepOffTheSet();
	void BringOnSet(EPropKind InKind, const FVector2D& Where);
	bool bShopSlot = false;

private:
	enum class EState : uint8
	{
		Resting,
		Held,
		Flying,
		Shattered
	};

	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void HandleFighterDown(AActor* Fighter, bool bPlayer);

	void Shatter();
	// What a prop leaves where it breaks: pieces that fly apart, land, and lie there until the next take.
	void BurstIntoPieces(const FVector& From, const FVector& Carried);
	void FlyPieces(float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	EPropKind Kind = EPropKind::Crate;
	EState State = EState::Resting;
	bool bPuppet = false;
	bool bCanStillHit = false;
	FTransform Home;
	// Let fall by the props crew at the start of a take, it comes down where it belongs, which may be a counter top.
	bool bDroppingIn = false;
	// Whoever carries it, and after a throw whoever threw it.
	TWeakObjectPtr<AStickFighter> Holder;
	int32 ThrowerTeam = INDEX_NONE;
	int32 DropOnKnockout = 0;
	int32 LatestKnockout = 0;
	// Thrown, as opposed to let fall. A legendary prop that was thrown is gone once it lands.
	bool bThrown = false;
	bool bInReserve = false;
	bool bOffTheSet = false;
	int32 KnockoutsSeen = 0;
	int32 UsesLeft = 0;
	int32 RoundsLeft = 0;
	float LastShotSeconds = -10.f;
	TWeakObjectPtr<AActor> ReservedFor;
	FVector Velocity = FVector::ZeroVector;
	double FloorHeight = 0.;
	double HalfHeight = 0.;
	double HalfLength = 0.;
	FVector2D ArenaMin = FVector2D::ZeroVector;
	FVector2D ArenaMax = FVector2D::ZeroVector;

	// A prop that has come to rest rights itself over a moment, from however it was lying when it stopped.
	FRotator SettleFrom = FRotator::ZeroRotator;
	float SettleSecondsLeft = 0.f;
	// Each bounce takes some of the spin out of it.
	float SpinShare = 1.f;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PieceMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> PieceMaterial;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;
	TArray<FVector> PieceVelocities;
	bool bPiecesFlying = false;
	// How the puppet moved between its last two places, for the pieces of one that breaks in the film.
	FVector PuppetVelocity = FVector::ZeroVector;
};
