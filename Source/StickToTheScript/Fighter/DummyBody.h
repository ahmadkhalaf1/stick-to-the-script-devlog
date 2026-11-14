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
	void PoseOn(const TArray<FVector>& Joints, double FloorHeight, float Girth, uint8 Severed = 0);
	void Paint(const FLinearColor& Skin, const FLinearColor& Rubber);

private:
	bool LearnRest();

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
