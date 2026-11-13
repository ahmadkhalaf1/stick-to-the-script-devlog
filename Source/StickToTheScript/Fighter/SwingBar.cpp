#include "StickToTheScript/Fighter/SwingBar.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Of an engine cylinder one metre across and one metre long.
	const double PipeThickness = 0.11;
	const double HangerThickness = 0.035;
	const double HangerLength = 420.;
	// Scaffold red, so it reads as something to use and not as part of the set.
	const FLinearColor PipeColor(1.5f, 0.2f, 0.06f);
	const FLinearColor HangerColor(0.05f, 0.05f, 0.06f);
	const float GlowCandelas = 700.f;
	const float GlowReach = 320.f;
	// Under the pipe and towards the camera, which in every stage looks from the +Y side.
	const FVector GlowOffset(0., 70., -38.);
}

ASwingBar::ASwingBar()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));

	Pipe = CreateDefaultSubobject<UStaticMeshComponent>("Pipe");
	RootComponent = Pipe;
	Pipe->SetStaticMesh(Cylinder.Object);
	Pipe->SetMaterial(0, Material.Object);
	Pipe->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	for (int32 Index = 0; Index < 2; ++Index)
	{
		UStaticMeshComponent* Hanger = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Hanger%d"), Index));
		Hanger->SetupAttachment(Pipe);
		Hanger->SetUsingAbsoluteRotation(true);
		Hanger->SetUsingAbsoluteScale(true);
		Hanger->SetStaticMesh(Cylinder.Object);
		Hanger->SetMaterial(0, Material.Object);
		Hanger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Hangers.Add(Hanger);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>("Glow");
	Glow->SetupAttachment(Pipe);
	Glow->SetUsingAbsoluteRotation(true);
	Glow->SetUsingAbsoluteScale(true);
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(GlowCandelas);
	Glow->SetAttenuationRadius(GlowReach);
	Glow->SetLightColor(FLinearColor(1.f, 0.85f, 0.7f));
	Glow->SetCastShadows(false);
}

void ASwingBar::SetUp(double HalfLength, double PostHeight)
{
	// The engine's cylinder stands upright. Rolled over, it lies across the set.
	SetActorRotation(FRotator(0., 0., 90.));
	Pipe->SetWorldScale3D(FVector(PipeThickness, PipeThickness, HalfLength * 2. / 100.));
	Pipe->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", PipeColor);

	Glow->SetWorldLocation(GetActorLocation() + GlowOffset);

	for (int32 Index = 0; Index < Hangers.Num(); ++Index)
	{
		const double Across = (Index == 0 ? -1. : 1.) * (HalfLength - 12.);
		// The far end is the one away from the camera, so a post there hides nothing.
		const bool bPost = PostHeight > 0.;
		const double Length = bPost ? PostHeight : HangerLength;
		const double Thickness = bPost ? PipeThickness : HangerThickness;
		Hangers[Index]->SetVisibility(!bPost || Index == 0);
		Hangers[Index]->SetWorldLocation(GetActorLocation() + FVector(0., Across, bPost ? -Length * 0.5 : Length * 0.5));
		Hangers[Index]->SetWorldScale3D(FVector(Thickness, Thickness, Length / 100.));
		Hangers[Index]->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", bPost ? PipeColor : HangerColor);
	}
}
