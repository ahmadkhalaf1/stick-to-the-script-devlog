#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "StickPlayerController.generated.h"

class AStickFighter;
class UInputMappingContext;

UCLASS()
class AStickPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	// Exec, so "SubmitMove Punch" in the console strikes without a keyboard or gamepad.
	UFUNCTION(Exec)
	void SubmitMove(EMove Move);

	void JumpOrContinue();
	void GrabOrContinue();
	void ChangeLook(int32 Row, int32 Direction);
	void ToggleView();
	void TogglePause();
	void SetAutoplay(bool bOn);

	// For looking at a move: every fighter moves at a fifth of its speed, or at full speed again.
	void ToggleSlowFighters();

	// For tests in the editor: the opponents all but stand still, so a check is not undone by the fight.
	void ToggleOpponentsHeld();

	// For tests in the editor, which can tap a key but not hold one: the duck key counts as held, then the guard key, then neither.
	void CycleHeldForTests();
	void HurtEveryoneForTests();

	// For tests and demos: with autoplay on, the keys 1, 2 and 3 go straight to that scene.
	void JumpToScene(int32 Index);

	// The big finish, once its bar is full and somebody is near. The film's low camera takes the picture for it.
	void TryFinish();
	// 0 to 1: how full the bar is. Blows that land fill it.
	float GetFinishCharge() const { return FinishCharge; }

	// The fighter the player walks around the fight.
	UPROPERTY()
	TObjectPtr<AStickFighter> Fighter;

protected:
	virtual void SetupInputComponent() override;

private:
	UFUNCTION()
	void HandleAction();

	UFUNCTION()
	void HandleCut(ECutReason Reason);

	UFUNCTION()
	void HandleTakeFinished(const FTake& Take);

	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void HandleHit(AActor* Attacker, AActor* Victim, EMove Move);

	UFUNCTION()
	void HandleFinish(AActor* Attacker, int32 Beat);

	float FinishCharge = 0.f;
	// The finish's own blows do not fill the bar it has just emptied.
	double FinishOwnBlowsUntilRealSeconds = 0.;
	// The view the finish cut away from, and when to cut back to it.
	UPROPERTY()
	TObjectPtr<AActor> ViewBeforeFinish;
	UPROPERTY()
	TObjectPtr<AActor> FinishCamera;
	double FinishShotUntilRealSeconds = 0.;
	void EndFinishShot();

	// False while paused: the keys then answer the pause menu and must not reach the fighter.
	bool IsFighting() const;
	AStickFighter* NearestOpponent(double Within) const;
	void PlayItself();

	UPROPERTY()
	TObjectPtr<UInputMappingContext> MoveMapping;

	bool bFightLive = false;
	int32 AutoplayStrikes = 0;
	// When the game's own player last had no loaded gun in its hands: it waits a moment after picking one up.
	float AutoplayUnarmedSeconds = 0.f;
	// What the game's own player has done in this take, and until when it is catching its breath.
	int32 AutoplayStrikesThisTake = 0;
	int32 AutoplayShotsThisTake = 0;
	float AutoplayRestUntilSeconds = 0.f;
	bool bAutoplayGuards = false;
	// Since when the finish bar has been full, for the game's own player. Negative while it is not.
	float AutoplayFinishFullSinceSeconds = -1.f;
	// Strikes and jumps asked for before the fighter could do them, in the order they were asked for. Each is done the
	// instant it can be, so keys pressed during the move before are not lost, and grows stale if that takes too long.
	struct FAskedFor
	{
		// None stands for a jump.
		EMove Move = EMove::None;
		float Until = 0.f;
		// The key that was down when it was asked for: Duck, Block or None.
		EMove Held = EMove::None;
	};
	TArray<FAskedFor> AskedFor;
	bool bWasDefending = false;
	void AskFor(EMove Move, EMove Held = EMove::None);
	// Duck, Block or None: which of the two held keys is down. Ducking wins when both are.
	EMove HeldKey() const;
	EMove HeldForTests = EMove::None;
	bool bSlowFighters = false;
	bool bOpponentsHeld = false;
};
