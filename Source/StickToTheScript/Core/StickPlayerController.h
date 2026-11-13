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

	// For tests and demos: with autoplay on, the keys 1, 2 and 3 go straight to that scene.
	void JumpToScene(int32 Index);

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

	// False while paused: the keys then answer the pause menu and must not reach the fighter.
	bool IsFighting() const;
	AStickFighter* NearestOpponent(double Within) const;
	void PlayItself();

	UPROPERTY()
	TObjectPtr<UInputMappingContext> MoveMapping;

	bool bFightLive = false;
	int32 AutoplayStrikes = 0;
	bool bSlowFighters = false;
};
