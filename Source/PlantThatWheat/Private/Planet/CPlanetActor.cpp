#include "CPlanetActor.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"
#include "Engine/Classes/Engine/StaticMesh.h"
#include "CPlanetProceduralMesh.h"
#include "CGroundSection.h"
#include "ProceduralMeshComponent.h"
#include "CCapture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"

#include "Components/SphereComponent.h"
#include "PlantThatWheat.h"

// ObjectInitializer Constructor used intstead of ACPlanetActor() for PlanetActor Constructor compatibility.
ACPlanetActor::ACPlanetActor(const FObjectInitializer& Objectinititializer) {
	StaticMeshScale = 254;
	HexGridOffset = 5;
	GroundSectionMaterial = CreateDefaultSubobject<UMaterial>(TEXT("GroundSectionMaterial"));

	bUseCaptureComponent = false;
}

void ACPlanetActor::BeginPlay() {
	Super::BeginPlay();

	// Set Dyanmic Material:
	UMaterialInterface* MatMesh = MeshComponent->GetMaterial(0);
	DynamicMaterial = UMaterialInstanceDynamic::Create(MatMesh, MeshComponent);
	MeshComponent->SetMaterial(0, DynamicMaterial);

	HexGridScale = (SphereCollisionRadius * GetActorScale().X) + HexGridOffset; // Assumption: perfectly round planet (X/Y/Z).

	if (bUseCaptureComponent) {
		InitCaptureComponent();
	}
	
	InitHexGrid();
}

void ACPlanetActor::InitHexGrid() {
	FTransform AdjustedTransform = GetTransform();
	int32 curVertex = 0; // Start at beginning of HexVertices.
	
	AdjustedTransform.SetScale3D(FVector(HexGridScale, HexGridScale, HexGridScale));

	if (GetWorld() != NULL) {
		ProcBoundingMesh = ACPlanetProceduralMesh::CREATE(GetWorld(), AdjustedTransform, false, false); // Calculates all of the values for the hex grid.
	}
	else return;

	// 12.27.18 Create Procedural Mesh section after filling array of vertices with faces.
	HexGrid = ACGroundSection::CREATE(GetWorld(), AdjustedTransform, ProcBoundingMesh->HexVertices, ProcBoundingMesh->FaceSequence,
		GroundSectionMaterial, HexGridScale, WheatComp, this);//->ProcMeshComp->SetMaterial(0, GroundSectionMaterial);
}

void ACPlanetActor::DisableSphereCollision()
{
	// CollisionType should not change at run-time.
	if (CollisionType == ECollisionType::ECol_Sphere) {
		SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ACPlanetActor::EnableSphereCollision() {
	// Only re-enable if sphere collision was previously active.
	if (CollisionType == ECollisionType::ECol_Sphere) {
		SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		SphereCollision->SetCollisionResponseToChannel(COLLISION_DIGTRACE, ECollisionResponse::ECR_Ignore);
	}
}

void ACPlanetActor::InitCaptureComponent() {
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CaptureComp = GetWorld()->SpawnActor<ACCapture>(CaptureCompClass, FVector::ZeroVector, FRotator(90, 0, 0), SpawnParams);
	if (CaptureComp) {
		if (StoredMaterial != nullptr) {
			DynamicMaterial = UMaterialInstanceDynamic::Create(StoredMaterial, MeshComponent);

			for (int i = 0; i < CaptureComp->NUM_RT; i++) {
				if (i > CaptureComp->QuadrantNames.Num()) break;

				FString QName = CaptureComp->QuadrantNames[i];
				FString RT = "RT_" + QName;
				FString OX = "Ortho_" + QName + "_X";
				FString OY = "Ortho_" + QName + "_Y";
				FString OZ = "Ortho_" + QName + "_Z";

				DynamicMaterial->SetTextureParameterValue(FName(*RT), CaptureComp->GetRenderTargetByIndex(i));
				DynamicMaterial->SetVectorParameterValue(FName(*OX), CaptureComp->GetOrthonormalBaseX(i));
				DynamicMaterial->SetVectorParameterValue(FName(*OY), CaptureComp->GetOrthonormalBaseY(i));
				DynamicMaterial->SetVectorParameterValue(FName(*OZ), CaptureComp->GetOrthonormalBaseZ(i));
			}

			MeshComponent->SetMaterial(0, DynamicMaterial);
		}
	}
}