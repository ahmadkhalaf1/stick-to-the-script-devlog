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

	const uint8 TurnGiven = 1;
	const uint8 PlaceGiven = 2;

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

void UDummyBody::PoseOn(const TArray<FVector>& Joints, double FloorHeight, float Girth)
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
	const double Size = FMath::Max(Leg / RestLeg, 0.2) * Girth;
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
	Put(DummyHead, HeadTurn, Joints[SkullJoint]);

	for (int32 Side = 0; Side < 2; ++Side)
	{
		// An arm. The elbow bends forward, so at rest its axis is the upper arm crossed with the way the model faces.
		const int32 Arm = Side == 0 ? DummyLeftArm : DummyRightArm;
		const FVector Shoulder = Joints[Side == 0 ? LeftShoulder : RightShoulder];
		const FVector Elbow = Joints[Side == 0 ? LeftElbow : RightElbow];
		const FVector Hand = Joints[Side == 0 ? LeftHand : RightHand];
		const FVector Inward = (Side == 0 ? ShoulderLine : -ShoulderLine).GetSafeNormal();
		const FVector ElbowAxis = DummyHinge(Elbow - Shoulder, Hand - Elbow, Front + Up * 0.6 + Inward * 0.3);
		const FVector RestUpperArm = RestAt(Arm + 1) - RestAt(Arm);
		const FVector RestForearm = RestAt(Arm + 2) - RestAt(Arm + 1);
		const FQuat UpperArmTurn = DummyFrame(Elbow - Shoulder, ElbowAxis) * DummyFrame(RestUpperArm, RestUpperArm ^ RestFront).Inverse();
		const FQuat ForearmTurn = DummyFrame(Hand - Elbow, ElbowAxis) * DummyFrame(RestForearm, RestForearm ^ RestFront).Inverse();
		Put(Arm, UpperArmTurn, Shoulder);
		Put(Arm + 1, ForearmTurn, Elbow);
		Put(Arm + 2, ForearmTurn, Hand);
		for (const FFingerBone& Finger : Fingers[Side])
		{
			Turn(Finger.Bone, ForearmTurn * Finger.Curl);
		}

		// A leg. The knee bends backward.
		const int32 Thigh = Side == 0 ? DummyLeftUpLeg : DummyRightUpLeg;
		const FVector Hip = Joints[Side == 0 ? LeftHip : RightHip];
		const FVector Knee = Joints[Side == 0 ? LeftKnee : RightKnee];
		const FVector Ankle = Joints[Side == 0 ? LeftFoot : RightFoot];
		const FVector KneeAxis = DummyHinge(Knee - Hip, Ankle - Knee, -Front - Up * 0.6);
		const FVector RestThigh = RestAt(Thigh + 1) - RestAt(Thigh);
		const FVector RestShin = RestAt(Thigh + 2) - RestAt(Thigh + 1);
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

	// Every bone in turn, parents first. One that was given nothing keeps its place on its parent, which is how the
	// collar bones, the toes and the top of the head come along.
	for (int32 Bone = 0; Bone < Rest.Num(); ++Bone)
	{
		const int32 Parent = Parents[Bone];
		FTransform Now = Parent == INDEX_NONE ? RestLocal[Bone] : RestLocal[Bone] * Posed[Parent];
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

void UDummyBody::Paint(const FLinearColor& Skin, const FLinearColor& Rubber)
{
	if (!HasBody())
	{
		return;
	}
	CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Skin);
	CreateDynamicMaterialInstance(1)->SetVectorParameterValue("Color", Rubber);
}
