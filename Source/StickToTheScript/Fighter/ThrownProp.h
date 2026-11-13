#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "ThrownProp.generated.h"

class AStickFighter;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

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

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	EPropKind Kind = EPropKind::Crate;
	EState State = EState::Resting;
	bool bPuppet = false;
	bool bCanStillHit = false;
	FTransform Home;
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
	FVector Velocity = FVector::ZeroVector;
	double FloorHeight = 0.;
	double HalfHeight = 0.;
	double HalfLength = 0.;
	FVector2D ArenaMin = FVector2D::ZeroVector;
	FVector2D ArenaMax = FVector2D::ZeroVector;
};
