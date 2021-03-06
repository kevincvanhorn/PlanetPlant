

#include "CDigTool.h"

#include "CProjectileActor.h"
#include "Engine/World.h"

#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CCharacterBase.h"

#include "CMultiTool.h"
#include "DrawDebugHelpers.h"
#include "PlantThatWheat.h"

#include "CLevelScriptActor.h"

#include "CPlanetActor.h"


ACDigTool::ACDigTool() {
	bCanDamage = false;
	MuzzleSocketName = "MuzzleFlashSocket";

	HoleRadius = 200;
	HoleHardness = 0.5f;
}

void ACDigTool::Init(ACLevelScriptActor * Level)
{
	this->Level = Level;
	if (Level) {
		this->Planet = Level->Planet;
	}
	this->OnInit();
}

void ACDigTool::Activate()
{
	Super::Activate();
	if (MyOwner) {
		MyOwner->BeginZoom();
	}
}

void ACDigTool::Deactivate()
{
	if (MyOwner) {
		MyOwner->EndZoom();
	}
	Super::Deactivate();
}

void ACDigTool::Interact()
{
	if (!bCanSingleTrace)
		return;

	// Trace from Pawn to center of screen (crosshair).
	AActor* MyOwner = GetOwner();

	if (MyOwner) {
		// Trace Starting Transform
		FVector EyeLocation;
		FRotator EyeRotation;

		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation); // Fills Eye-prefixed vars w/ data from owning actor.
		FVector TraceDirection = EyeRotation.Vector(); // For Damage functions.
		FVector TraceEnd = EyeLocation + (TraceDirection * SingleTraceDist); // Trace EndLocation

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true; // Trace against each triangle of the mesh - ie. exact location. 
		QueryParams.bReturnPhysicalMaterial = true;
		QueryParams.bReturnFaceIndex = true;

		FVector TracerEndPoint = TraceEnd; // Effect endpoint paramater.

		FHitResult Hit; // Struct filled with hit data.
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_DIGTRACE, QueryParams)) {
			// Blocking hit, Process damage. 

			AActor * HitActor = Hit.GetActor();

			// Apply Point Damage
			if (bCanDamage) {
				ApplyDamage(HitActor, TraceDirection, Hit, MyOwner->GetInstigatorController());
			}

			TracerEndPoint = Hit.ImpactPoint;

			FVector2D UV;
			if (UGameplayStatics::FindCollisionUV(Hit, 0, UV)) {
				this->OnUVTrace(UV, Hit.Location);

				AddDigArea(Hit.Location);

				UE_LOG(LogTemp, Warning, TEXT("UV HIT --------- %s"), *UV.ToString());
				UE_LOG(LogTemp, Warning, TEXT("UV HIT --------- %s"), *Hit.ToString());
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("UV HIT FAIL"));
			}
		}

		DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::Red, false, 1.0f, 0, 1.0f); // Trace Debug Line
	}
}

void ACDigTool::BeginPlay()
{
	Super::BeginPlay();
	//Level & Planet are set after BeginPlay() via CCharacter.
	
	// 0 Hardness is soft, 1 is simply the radius
	AdjustedHoleRadius = HoleRadius - 0.9*((1-HoleHardness)*HoleRadius); // 0.8 is the threshold. lower = closer to original radius.
}

void ACDigTool::AddDigArea(FVector & DigCentroid)
{
	if (Level) {
		Level->AddDigArea(DigCentroid, AdjustedHoleRadius);
	}
}