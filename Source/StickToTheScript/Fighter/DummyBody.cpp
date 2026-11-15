#include "StickToTheScript/Fighter/DummyBody.h"

#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ReferenceSkeleton.h"
#include "StickToTheScript/Fighter/StickFighter.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	using namespace StickJoint;

	// The bones of the model that are placed, by the names Tools/build_character.py checks for.
	enum EDummyBone
	{
		DummyHips, DummySpine, DummySpine1, DummySpine2, DummyNeck, DummyHead,
		DummyLeftArm, DummyLeftForeArm, DummyLeftHand, DummyRightArm, DummyRightForeArm, DummyRightHand,
		DummyLeftUpLeg, DummyLeftLeg, DummyLeftFoot, DummyRightUpLeg, DummyRightLeg, DummyRightFoot,
		DummyBoneCount
	};
	const TCHAR* const DummyBoneNames[DummyBoneCount] = {
		TEXT("Hips"), TEXT("Spine"), TEXT("Spine1"), TEXT("Spine2"), TEXT("Neck"), TEXT("Head"),
		TEXT("LeftArm"), TEXT("LeftForeArm"), TEXT("LeftHand"), TEXT("RightArm"), TEXT("RightForeArm"), TEXT("RightHand"),
		TEXT("LeftUpLeg"), TEXT("LeftLeg"), TEXT("LeftFoot"), TEXT("RightUpLeg"), TEXT("RightLeg"), TEXT("RightFoot"),
	};
	const TCHAR* const DummyFingerNames[] = { TEXT("Thumb"), TEXT("Index"), TEXT("Middle"), TEXT("Ring"), TEXT("Pinky") };

	// How far each bone of a finger is turned from straight to make a fist, in degrees: the thumb, then the others.
	const float ThumbCurl[] = { 20.f, 45.f, 70.f };
	const float FingerCurl[] = { 70.f, 150.f, 200.f };

	// A foot whose ankle is this close to the floor, in centimetres, lies flat on it. Higher than the second, it follows its shin.
	const double FlatFootUnder = 14.;
	const double LooseFootOver = 30.;

	// How high a fighter's ankle is above the floor it stands on, and how much of a fighter's build one of the cast takes on.
	const double UsualAnkleHeight = 12.;
	const float CastGirthShare = 0.35f;
	// How much longer than it was made a limb's bone may be drawn. Some of the cast have shorter legs than the
	// fighter they dress, and need most of this just to stand.
	const double MostStretch = 1.35;
	// How far a broken arm's hand is turned off the line of its forearm, sideways, where no wrist bends.
	const float BrokenWristDegrees = 75.f;

	// The bone that stands at each of the fighter's joints, in the order of StickJoint.
	const int32 BoneOfJoint[JointCount] = {
		DummyHips, DummySpine2, DummyNeck, DummyHead,
		DummyLeftArm, DummyLeftForeArm, DummyLeftHand, DummyRightArm, DummyRightForeArm, DummyRightHand,
		DummyLeftUpLeg, DummyLeftLeg, DummyLeftFoot, DummyRightUpLeg, DummyRightLeg, DummyRightFoot,
	};

	const uint8 TurnGiven = 1;
	const uint8 PlaceGiven = 2;
	const uint8 GoneGiven = 4;

	// A turn that has its X along a bone and its Z as near as it can be to the axis the bone's joint bends about.
	FQuat DummyFrame(const FVector& Along, const FVector& Hinge)
	{
		return FRotationMatrix::MakeFromXZ(Along.GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector), Hinge).ToQuat();
	}

	// The axis an elbow or a knee bends about, from the two bones that meet there. A straight limb has none, so it
	// leans on a guess of which way it would bend, which also keeps the axis from flipping as the limb straightens.
	FVector DummyHinge(const FVector& Upper, const FVector& Lower, const FVector& WouldBendTowards)
	{
		const FVector First = Upper.GetSafeNormal();
		const FVector Guess = (First ^ WouldBendTowards).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
		return ((First ^ Lower.GetSafeNormal()) + Guess * 0.06).GetSafeNormal(UE_SMALL_NUMBER, Guess);
	}
}

UDummyBody::UDummyBody()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Model(TEXT("/Game/Characters/Dummy/SK_Dummy"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Materials/M_Figure"));
	// The engine's own way of setting it, which also tells the component that it changed: writing it directly fails the cook.
	SetSkinnedAssetAndUpdate(Model.Object);
	SetMaterial(0, Material.Object);
	SetMaterial(1, Material.Object);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// It is put where the joints are, wherever the actor is: a body knocked across the set, or replayed in the film,
	// is not where its actor stands.
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	SetUsingAbsoluteScale(true);
}

bool UDummyBody::LearnRest()
{
	if (Placed.Num() == DummyBoneCount)
	{
		return true;
	}
	const USkinnedAsset* Model = GetSkinnedAsset();
	if (!Model)
	{
		return false;
	}

	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	RestLocal = Skeleton.GetRefBonePose();
	Rest.SetNum(RestLocal.Num());
	Parents.SetNum(RestLocal.Num());
	for (int32 Bone = 0; Bone < RestLocal.Num(); ++Bone)
	{
		Parents[Bone] = Skeleton.GetParentIndex(Bone);
		Rest[Bone] = Parents[Bone] == INDEX_NONE ? RestLocal[Bone] : RestLocal[Bone] * Rest[Parents[Bone]];
	}

	TArray<int32> Found;
	for (const TCHAR* Name : DummyBoneNames)
	{
		const int32 Bone = Skeleton.FindBoneIndex(FName(Name));
		if (Bone == INDEX_NONE)
		{
			return false;
		}
		Found.Add(Bone);
	}

	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FVector Hand = Rest[Found[Side == 0 ? DummyLeftHand : DummyRightHand]].GetTranslation();
		for (int32 Finger = 0; Finger < UE_ARRAY_COUNT(DummyFingerNames); ++Finger)
		{
			FVector Along = FVector::ZeroVector;
			for (int32 Part = 0; Part < 3; ++Part)
			{
				const int32 Bone = Skeleton.FindBoneIndex(FName(*FString::Printf(TEXT("%sHand%s%d"), Side == 0 ? TEXT("Left") : TEXT("Right"), DummyFingerNames[Finger], Part + 1)));
				if (Bone == INDEX_NONE)
				{
					break;
				}
				if (Part == 0)
				{
					Along = (Rest[Bone].GetTranslation() - Hand).GetSafeNormal();
				}
				// A finger closes towards the palm, which faces the floor while the arms are held straight out.
				const FVector About = (Along ^ FVector::DownVector).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
				Fingers[Side].Add({ Bone, FQuat(About, FMath::DegreesToRadians(Finger == 0 ? ThumbCurl[Part] : FingerCurl[Part])) });
			}
		}
	}

	Posed.SetNum(Rest.Num());
	Turns.SetNum(Rest.Num());
	Places.SetNum(Rest.Num());
	Given.SetNum(Rest.Num());
	Placed = MoveTemp(Found);
	return true;
}

bool UDummyBody::FindDrawnJoint(int32 Joint, FVector& At) const
{
	if (Placed.Num() != DummyBoneCount || !Posed.IsValidIndex(Placed[BoneOfJoint[Joint]]))
	{
		return false;
	}
	At = GetComponentTransform().TransformPosition(Posed[Placed[BoneOfJoint[Joint]]].GetTranslation());
	return true;
}

void UDummyBody::PoseOn(const TArray<FVector>& Joints, double FloorHeight, float Girth, uint8 Severed, uint8 BrokenArms)
{
	if (Joints.Num() != JointCount || !LearnRest() || BoneSpaceTransforms.Num() != Rest.Num())
	{
		return;
	}

	auto RestAt = [this](int32 Bone) { return Rest[Placed[Bone]].GetTranslation(); };

	// The model is sized so that its legs are as long as the fighter's, then made stouter or slighter. Its bones are
	// put on the joints whatever its size, so the size only decides how thick it is and how big its hands, feet and head.
	const double RestLeg = FVector::Dist(RestAt(DummyLeftUpLeg), RestAt(DummyLeftLeg)) + FVector::Dist(RestAt(DummyLeftLeg), RestAt(DummyLeftFoot));
	const double Leg = FVector::Dist(Joints[LeftHip], Joints[LeftKnee]) + FVector::Dist(Joints[LeftKnee], Joints[LeftFoot]);
	// One of the cast is sized by how high its hips stand, and is only a little stouter for the build of whoever
	// wears it. Sized by its legs, a character with short legs under a long body came out half as big again as the rest.
	const double Size = bOwnSkin
		? FMath::Max((Leg + UsualAnkleHeight) / FMath::Max(RestAt(DummyHips).Z, 1.), 0.2) * FMath::Lerp(1.f, Girth, CastGirthShare)
		: FMath::Max(Leg / RestLeg, 0.2) * Girth;
	LastSize = Size;
	const FVector Origin = Joints[PelvisJoint] - FVector(0., 0., RestAt(DummyHips).Z * Size);
	SetWorldTransform(FTransform(FQuat::Identity, Origin, FVector(Size)));

	FMemory::Memzero(Given.GetData(), Given.Num());
	auto Turn = [this](int32 Bone, const FQuat& By)
	{
		Turns[Bone] = By;
		Given[Bone] |= TurnGiven;
	};
	auto Put = [this, &Turn](int32 Which, const FQuat& By, const FVector& At)
	{
		const int32 Bone = Placed[Which];
		Turn(Bone, By);
		Places[Bone] = At;
		Given[Bone] |= PlaceGiven;
	};

	const FVector Up = FVector::UpVector;
	const FVector RestSide = RestAt(DummyRightArm) - RestAt(DummyLeftArm);
	const FVector RestFront = (RestSide ^ Up).GetSafeNormal();
	const FVector HipLine = Joints[RightHip] - Joints[LeftHip];
	const FVector ShoulderLine = Joints[RightShoulder] - Joints[LeftShoulder];
	const FVector Front = (ShoulderLine ^ (Joints[NeckJoint] - Joints[PelvisJoint])).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);

	// The trunk: the hips keep their shape round the two hip joints and the chest round the two shoulders, and the
	// spine between them takes up whatever difference there is between this model's back and the fighter's.
	const FQuat HipsTurn = DummyFrame(HipLine, Joints[ChestJoint] - Joints[PelvisJoint])
		* DummyFrame(RestAt(DummyRightUpLeg) - RestAt(DummyLeftUpLeg), RestAt(DummySpine1) - RestAt(DummyHips)).Inverse();
	const FQuat ChestTurn = DummyFrame(ShoulderLine, Joints[NeckJoint] - Joints[ChestJoint])
		* DummyFrame(RestSide, RestAt(DummyNeck) - RestAt(DummySpine1)).Inverse();
	const FQuat HeadTurn = FRotationMatrix::MakeFromZX(Joints[SkullJoint] - Joints[NeckJoint], ShoulderLine).ToQuat()
		* FRotationMatrix::MakeFromZX(RestAt(DummyHead) - RestAt(DummyNeck), RestSide).ToQuat().Inverse();
	const FVector HipsAt = (Joints[LeftHip] + Joints[RightHip]) * 0.5
		+ HipsTurn.RotateVector(RestAt(DummyHips) - (RestAt(DummyLeftUpLeg) + RestAt(DummyRightUpLeg)) * 0.5) * Size;
	const FVector ChestAt = (Joints[LeftShoulder] + Joints[RightShoulder]) * 0.5
		+ ChestTurn.RotateVector(RestAt(DummySpine2) - (RestAt(DummyLeftArm) + RestAt(DummyRightArm)) * 0.5) * Size;
	Put(DummyHips, HipsTurn, HipsAt);
	Put(DummySpine, FQuat::Slerp(HipsTurn, ChestTurn, 1. / 3.), FMath::Lerp(HipsAt, ChestAt, 1. / 3.));
	Put(DummySpine1, FQuat::Slerp(HipsTurn, ChestTurn, 2. / 3.), FMath::Lerp(HipsAt, ChestAt, 2. / 3.));
	Put(DummySpine2, ChestTurn, ChestAt);
	Put(DummyNeck, FQuat::Slerp(ChestTurn, HeadTurn, 0.5), Joints[NeckJoint]);
	// A neck is drawn no longer than a neck can be, as a limb is: a body thrown into the air had one a forearm long.
	Put(DummyHead, HeadTurn, Joints[NeckJoint]
		+ (Joints[SkullJoint] - Joints[NeckJoint]).GetClampedToMaxSize(FVector::Dist(RestAt(DummyHead), RestAt(DummyNeck)) * Size * MostStretch));

	for (int32 Side = 0; Side < 2; ++Side)
	{
		// An arm. The elbow bends forward, so at rest its axis is the upper arm crossed with the way the model faces.
		const int32 Arm = Side == 0 ? DummyLeftArm : DummyRightArm;
		const FVector RestUpperArm = RestAt(Arm + 1) - RestAt(Arm);
		const FVector RestForearm = RestAt(Arm + 2) - RestAt(Arm + 1);
		// A knocked-out body's joints can lie much further apart than a limb is long, and a limb drawn all that way
		// was a rubber hose. It is drawn no longer than a limb can be, and stops short of the joint.
		const FVector Shoulder = Joints[Side == 0 ? LeftShoulder : RightShoulder];
		const FVector Elbow = Shoulder + (Joints[Side == 0 ? LeftElbow : RightElbow] - Shoulder).GetClampedToMaxSize(RestUpperArm.Size() * Size * MostStretch);
		const FVector Hand = Elbow + (Joints[Side == 0 ? LeftHand : RightHand] - Elbow).GetClampedToMaxSize(RestForearm.Size() * Size * MostStretch);
		const FVector Inward = (Side == 0 ? ShoulderLine : -ShoulderLine).GetSafeNormal();
		const FVector ElbowAxis = DummyHinge(Elbow - Shoulder, Hand - Elbow, Front + Up * 0.6 + Inward * 0.3);
		const FQuat UpperArmTurn = DummyFrame(Elbow - Shoulder, ElbowAxis) * DummyFrame(RestUpperArm, RestUpperArm ^ RestFront).Inverse();
		const FQuat ForearmTurn = DummyFrame(Hand - Elbow, ElbowAxis) * DummyFrame(RestForearm, RestForearm ^ RestFront).Inverse();
		Put(Arm, UpperArmTurn, Shoulder);
		Put(Arm + 1, ForearmTurn, Elbow);
		// A model's forearm is one bone and cannot be drawn snapped in two, so its break shows at the wrist: the hand
		// hangs off sideways with its fingers open.
		const bool bBroken = (BrokenArms & (1 << Side)) != 0;
		const FQuat HandTurn = bBroken
			? FQuat(((Hand - Elbow) ^ ElbowAxis).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector), FMath::DegreesToRadians(BrokenWristDegrees)) * ForearmTurn
			: ForearmTurn;
		Put(Arm + 2, HandTurn, Hand);
		for (const FFingerBone& Finger : Fingers[Side])
		{
			Turn(Finger.Bone, bBroken ? HandTurn : HandTurn * Finger.Curl);
		}

		// A leg. The knee bends backward.
		const int32 Thigh = Side == 0 ? DummyLeftUpLeg : DummyRightUpLeg;
		const FVector RestThigh = RestAt(Thigh + 1) - RestAt(Thigh);
		const FVector RestShin = RestAt(Thigh + 2) - RestAt(Thigh + 1);
		const FVector Hip = Joints[Side == 0 ? LeftHip : RightHip];
		const FVector Knee = Hip + (Joints[Side == 0 ? LeftKnee : RightKnee] - Hip).GetClampedToMaxSize(RestThigh.Size() * Size * MostStretch);
		const FVector Ankle = Knee + (Joints[Side == 0 ? LeftFoot : RightFoot] - Knee).GetClampedToMaxSize(RestShin.Size() * Size * MostStretch);
		const FVector KneeAxis = DummyHinge(Knee - Hip, Ankle - Knee, -Front - Up * 0.6);
		const FQuat ThighTurn = DummyFrame(Knee - Hip, KneeAxis) * DummyFrame(RestThigh, RestThigh ^ -RestFront).Inverse();
		const FQuat ShinTurn = DummyFrame(Ankle - Knee, KneeAxis) * DummyFrame(RestShin, RestShin ^ -RestFront).Inverse();
		Put(Thigh, ThighTurn, Hip);
		Put(Thigh + 1, ShinTurn, Knee);

		// A foot follows its shin, as if the ankle were stiff, except under a standing leg: there it lies flat on the floor.
		const FVector Shin = (Ankle - Knee).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
		const FVector Toes = ((KneeAxis ^ Up) * FVector(1., 1., 0.)).GetSafeNormal(UE_SMALL_NUMBER, Front);
		const FQuat FlatTurn = DummyFrame(Toes, Up) * DummyFrame(RestFront, Up).Inverse();
		const double Flat = (1. - FMath::SmoothStep(FlatFootUnder, LooseFootOver, Ankle.Z - FloorHeight)) * FMath::SmoothStep(0.3, 0.7, -Shin.Z);
		Put(Thigh + 2, FQuat::Slerp(ShinTurn, FlatTurn, Flat), Ankle);
	}

	// A limb shot away is closed down at its joint along its own length only, so what is left ends in a flat cut as
	// wide as the limb was and not in a point. Its hand or foot closes down to nothing in the middle of the cut.
	const int32 CanGo[] = { DummyLeftForeArm, DummyRightForeArm, DummyLeftLeg, DummyRightLeg };
	int32 CutBones[4];
	FVector CutScales[4];
	for (int32 Limb = 0; Limb < 4; ++Limb)
	{
		CutBones[Limb] = INDEX_NONE;
		if (Severed & (1 << Limb))
		{
			CutBones[Limb] = Placed[CanGo[Limb]];
			Given[CutBones[Limb]] = GoneGiven;
			// The hand follows the forearm and the foot the shin in the list of bones, and lies along its length.
			const FVector Runs = RestLocal[Placed[CanGo[Limb] + 1]].GetTranslation().GetAbs();
			CutScales[Limb] = FVector(Runs.X < Runs.GetMax() ? 1. : 0.001, Runs.Y < Runs.GetMax() ? 1. : 0.001, Runs.Z < Runs.GetMax() ? 1. : 0.001);
		}
	}

	// Every bone in turn, parents first. One that was given nothing keeps its place on its parent, which is how the
	// collar bones, the toes and the top of the head come along.
	for (int32 Bone = 0; Bone < Rest.Num(); ++Bone)
	{
		const int32 Parent = Parents[Bone];
		// Whatever hangs from a bone that is gone goes with it, wherever its own joint says it should be.
		if (Parent != INDEX_NONE && (Given[Parent] & GoneGiven))
		{
			Given[Bone] = GoneGiven;
		}
		FTransform Now = Parent == INDEX_NONE ? RestLocal[Bone] : RestLocal[Bone] * Posed[Parent];
		if (Given[Bone] & GoneGiven)
		{
			const int32 Cut = MakeArrayView(CutBones).Find(Bone);
			Now.SetScale3D(Cut == INDEX_NONE ? FVector(0.001) : CutScales[Cut]);
		}
		if (Given[Bone] & TurnGiven)
		{
			Now.SetRotation((Turns[Bone] * Rest[Bone].GetRotation()).GetNormalized());
		}
		if (Given[Bone] & PlaceGiven)
		{
			Now.SetTranslation((Places[Bone] - Origin) / Size);
		}
		Posed[Bone] = Now;
		BoneSpaceTransforms[Bone] = Parent == INDEX_NONE ? Now : Now.GetRelativeTransform(Posed[Parent]);
	}
	RefreshBoneTransforms();
}

void UDummyBody::ShowOnlyLimb(int32 Limb)
{
	if (!LearnRest() || BoneSpaceTransforms.Num() != Rest.Num())
	{
		return;
	}
	const int32 LimbRoots[] = { DummyLeftForeArm, DummyRightForeArm, DummyLeftLeg, DummyRightLeg };
	const int32 Which = LimbRoots[FMath::Clamp(Limb, 0, 3)];
	const int32 Root = Placed[Which];
	// The hand follows the forearm and the foot the shin in the list of bones.
	LimbRootAtRest = Rest[Root].GetTranslation();
	LimbRunsAtRest = (Rest[Placed[Which + 1]].GetTranslation() - LimbRootAtRest).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);

	// Everything that is not the limb is closed down to nothing where the limb begins, which is how the body it came
	// off closes the limb down. Parents come before their children, so one pass tells what hangs from the limb's root.
	const FTransform Gone(Rest[Root].GetRotation(), LimbRootAtRest, FVector(0.001));
	TArray<bool> bKept;
	bKept.SetNumZeroed(Rest.Num());
	for (int32 Bone = 0; Bone < Rest.Num(); ++Bone)
	{
		const int32 Parent = Parents[Bone];
		bKept[Bone] = Bone == Root || (Parent != INDEX_NONE && bKept[Parent]);
		Posed[Bone] = bKept[Bone] ? Rest[Bone] : Gone;
		BoneSpaceTransforms[Bone] = Parent == INDEX_NONE ? Posed[Bone] : Posed[Bone].GetRelativeTransform(Posed[Parent]);
	}
	RefreshBoneTransforms();
}

void UDummyBody::PlaceLimb(const FVector& From, const FVector& To, double Size)
{
	const FQuat Turn = FQuat::FindBetweenNormals(LimbRunsAtRest, (To - From).GetSafeNormal(UE_SMALL_NUMBER, LimbRunsAtRest));
	SetWorldTransform(FTransform(Turn, From - Turn.RotateVector(LimbRootAtRest) * Size, FVector(Size)));
}

void UDummyBody::DyeOutfit(const FLinearColor& Dye)
{
	if (!bOwnSkin)
	{
		return;
	}
	// A material that is not cloth has no such colour to set, and is left as it is.
	for (int32 Slot = 0; Slot < GetNumMaterials(); ++Slot)
	{
		if (UMaterialInstanceDynamic* Cloth = CreateDynamicMaterialInstance(Slot))
		{
			Cloth->SetVectorParameterValue("Tint", Dye);
		}
	}
}

void UDummyBody::Wear(USkeletalMesh* Cast)
{
	if (!Cast || Cast == GetSkinnedAsset())
	{
		return;
	}
	SetSkinnedAssetAndUpdate(Cast);
	// The model's own materials, not the dummy's two.
	EmptyOverrideMaterials();
	// What was learnt of the dummy's bones is not true of this one.
	Placed.Reset();
	Fingers[0].Reset();
	Fingers[1].Reset();
	bOwnSkin = true;
}

void UDummyBody::Paint(const FLinearColor& Skin, const FLinearColor& Rubber)
{
	if (!HasBody() || bOwnSkin)
	{
		return;
	}
	CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Skin);
	CreateDynamicMaterialInstance(1)->SetVectorParameterValue("Color", Rubber);
}
