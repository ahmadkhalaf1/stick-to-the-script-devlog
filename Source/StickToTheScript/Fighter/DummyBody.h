#pragma once

#include "Components/PoseableMeshComponent.h"
#include "CoreMinimal.h"

#include "DummyBody.generated.h"

// The body a fighter is seen as: a skinned model whose bones are put, every frame, on the fighter's sixteen joints.
// The joints stay the truth of the game. This only dresses them, so every pose worked out on joints, and every
// take replayed from them, shows on the model without knowing it is there.
UCLASS()
class UDummyBody : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	UDummyBody();

	// False when the model is missing from the project. The fighter then draws itself from plain shapes, as before.
	bool HasBody() const { return GetSkinnedAsset() != nullptr; }

	// Joints as AStickFighter::GetJointPositions gives them. Girth is how stout the body is, 1 for the usual build.
	// Severed has a bit for each limb that is gone: the left and right forearm, then the left and right shin.
	// BrokenArms has one for the left and the right arm: a broken one's hand hangs loose and askew.
	void PoseOn(const TArray<FVector>& Joints, double FloorHeight, float Girth, uint8 Severed = 0, uint8 BrokenArms = 0);
	// Where the body, as it was last posed, has one of the fighter's joints. Not always where the joint is: a limb
	// is drawn no longer than it can be. False for a joint it has no bone for, or with no body.
	bool FindDrawnJoint(int32 Joint, FVector& At) const;
	// How far each hand is closed the next time the body is posed: 1 is a fist, less is a hand round something.
	void SetGrip(float Left, float Right) { Grip[0] = Left; Grip[1] = Right; }
	void Paint(const FLinearColor& Skin, const FLinearColor& Rubber);

	// Puts one of the cast on the same joints in the dummy's place: a model with a skin, a face and clothes of its
	// own, which Paint then leaves alone. Any model with the dummy's bone names will do.
	void Wear(class USkeletalMesh* Cast);
	bool WearsItsOwnSkin() const { return bOwnSkin; }
	// Dyes the clothes of one of the cast, where its materials are the kind that can be dyed (Tools/build_cast.py
	// makes the hero's so). The colour replaces what is dark and plain in its pictures: the cloth, not the skin.
	// Its alpha is how much of the dye is taken: none leaves the clothes as they were made.
	void DyeOutfit(const FLinearColor& Dye);

	// For a limb that has come off: shows that one limb of the model and nothing else, as it was made. 0 and 1 are
	// the forearms with their hands, 2 and 3 the shins with their feet. Then puts it where it has flown to: its
	// joint end at From, its hand or foot towards To, at the size the body it came off was drawn.
	void ShowOnlyLimb(int32 Limb);
	void PlaceLimb(const FVector& From, const FVector& To, double Size);
	double GetSize() const { return LastSize; }

private:
	bool LearnRest();
	bool bOwnSkin = false;
	double LastSize = 1.;
	float Grip[2] = { 1.f, 1.f };
	// Where the one limb shown starts in the model as it was made, and which way it runs.
	FVector LimbRootAtRest = FVector::ZeroVector;
	FVector LimbRunsAtRest = FVector::ForwardVector;

	// The model standing as it was made, arms straight out: each bone relative to its parent, and relative to the model.
	TArray<FTransform> RestLocal;
	TArray<FTransform> Rest;
	TArray<int32> Parents;
	TArray<int32> Placed;

	struct FFingerBone
	{
		int32 Bone = INDEX_NONE;
		// Closes it into a fist, as a turn of the bone where the model stands at rest.
		FQuat Curl = FQuat::Identity;
	};
	TArray<FFingerBone> Fingers[2];

	// Worked out afresh on every call, kept to save making them each frame.
	TArray<FTransform> Posed;
	TArray<FQuat> Turns;
	TArray<FVector> Places;
	TArray<uint8> Given;
};
