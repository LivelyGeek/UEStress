// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

#include "UEStressPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerInput.h"
#include "Materials/Material.h"
#include "Runtime/Engine/Public/EngineGlobals.h"

static int32 InfoState;
static int32 NaniteStatsState;
static int32 LumenState;

AUEStressPawn::AUEStressPawn()
{
	// This pawn only consists of a camera that flies around. The controls are the same as
	// in the editor (WASD for direction, QE for up/down, scroll wheel for speed). See bindings
	// in SetupPlayerInputComponent for all available commands.
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	RootComponent = CameraComponent;
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->MaxSpeed = 1024.f;
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;

	// Tick every frame to create any unbuilt meshes.
	PrimaryActorTick.bCanEverTick = true;

	// When 'b' is pressed, how many new meshes to add to the total to build.
	MeshCountPerBuild = 1000;

	// How many meshes to create per tick when there are unbuilt meshes. We don't create
	// all meshes at once because we want to be able to move around and see progress.
	MeshCountPerTick = 100;

	// How far apart to space meshes.
	MeshSpacing = 125.f;

	// Meshes are placed along Z, then Y, then X. These are when to wrap around on Z and Y.
	// X does not have a limit.
	MeshZMax = 10;
	MeshYMax = 100;

	// How many actors to create for every static mesh asset loaded below. If this is 0, one
	// actor is created for every mesh. If this is greater than 0, one component or instance
	// (depending on the value of bUseInstanced) is created on one of the pre-built actors.
	ActorsPerMesh = 1;
	bUseInstanced = true;
}

void AUEStressPawn::BeginPlay()
{
	Super::BeginPlay();

	// Material to apply to index 0 for static meshes. Comment out to not use.
	MeshMaterial = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), NULL,
		TEXT("/Game/Materials/White")));

	// Find all static meshes from content assets using specific name and numbering system.
	// Expected names are:
	//
	// /Content/Meshes/00_00
	// /Content/Meshes/00_01
	// /Content/Meshes/00_02
	// ...
	// /Content/Meshes/00_99
	// /Content/Meshes/01_00
	// /Content/Meshes/01_01
	// ...
	Meshes.Empty();
	int32 MeshIndex = 0;
	while (1)
	{
		UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL,
			*FString::Printf(TEXT("/Game/Meshes/%02d_%02d"), MeshIndex / 100, MeshIndex % 100)));
		if (!Mesh)
			break;
		Meshes.Add(Mesh);

		// Add a fixed number of actors if ActorsPerMesh is > 0, otherwise we'll add
		// one actor per mesh component/instance in Tick.
		for (int32 ActorIndex = 0; ActorIndex < ActorsPerMesh; ActorIndex++)
		{
			AddMeshActor(MeshIndex);
		}

		MeshIndex++;

		// To test with a single mesh, uncomment this break so only one mesh is added to the array.
		break;
	}

	RunCommand(TEXT("t.maxfps 240"));
	if (!InfoState)
		ToggleInfo();

	// This needs to be run before r.Nanite.ShowStats command works for some reason.
	RunCommand(TEXT("nanitestats"));
	if (!NaniteStatsState)
		ToggleNaniteStats();
	
	if (!LumenState)
		ToggleLumen();
}

void AUEStressPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (InfoState)
	{
		// Update status counters.
		GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::White,
			FString::Printf(TEXT("Meshes: %d/%d"), MeshCount, MeshCountTarget));

		GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::White,
			FString::Printf(TEXT("Lumen: %s (L to toggle)"), LumenState ? TEXT("on") : TEXT("off")));
	}

	int MeshCountThisTick = 0;

	while (MeshCount < MeshCountTarget)
	{
		if (ActorsPerMesh)
		{
			// We have a fixed number of actors, so either add an instance or mesh component.
			int32 ActorIndex = MeshCount % MeshActors.Num();
			AActor* Actor = MeshActors[ActorIndex];
			if (bUseInstanced)
			{
				Cast<UInstancedStaticMeshComponent>(Actor->GetRootComponent())->AddInstance(
					FTransform(FRotator::ZeroRotator, MeshLocation));
			}
			else
			{
				UStaticMeshComponent* Component = AddStaticMeshComponent<UStaticMeshComponent>(Actor,
					ActorIndex / ActorsPerMesh);
				Component->AttachToComponent(Actor->GetRootComponent(),
					FAttachmentTransformRules::KeepWorldTransform);
			}
		}
		else
		{
			// Create a new actor for every mesh.
			AddMeshActor(MeshCount % Meshes.Num());
		}

		// Calculate next mesh location, wrapping on Y and Z. X will extend forever.
		MeshZ++;
		if (MeshZ == MeshZMax)
		{
			MeshZ = 0;
			MeshY++;
			if (MeshY == MeshYMax)
			{
				MeshY = 0;
				MeshLocation.X += MeshSpacing;
			}
			MeshLocation.Y = MeshY * MeshSpacing;
		}
		MeshLocation.Z = MeshZ * MeshSpacing;

		MeshCount++;

		// Limit number of meshes created per tick.
		MeshCountThisTick++;
		if (MeshCountThisTick == MeshCountPerTick)
			return;
	}
}

AActor* AUEStressPawn::AddMeshActor(int32 MeshIndex)
{
	AActor* Actor = GetWorld()->SpawnActor<AActor>();
	MeshActors.Add(Actor);

	USceneComponent* Component;
	if (ActorsPerMesh == 0)
	{
		// Create one mesh per actor.
		Component = AddStaticMeshComponent<UStaticMeshComponent>(Actor, MeshIndex);
	}
	else if (bUseInstanced)
	{
		// Fixed number of actors, setup instanced mesh to add instances to later in tick.
		Component = AddStaticMeshComponent<UInstancedStaticMeshComponent>(Actor, MeshIndex);
	}
	else
	{
		// Fixed number of actors, setup scene component to attach mesh components to later in tick.
		Component = NewObject<USceneComponent>(Actor);
		Component->SetMobility(EComponentMobility::Static);
		Component->RegisterComponent();
	}

	Actor->SetRootComponent(Component);

	return Actor;
}

template<class T>
T* AUEStressPawn::AddStaticMeshComponent(class AActor* Actor, int32 MeshIndex)
{
	// Create static mesh or instanced static mesh and configure mesh and material from assets.
	T* Component = NewObject<T>(Actor);
	Component->SetMobility(EComponentMobility::Static);
	Component->SetStaticMesh(Meshes[MeshIndex]);
	if (MeshMaterial)
		Component->SetMaterial(0, MeshMaterial);
	Component->SetWorldLocation(MeshLocation);
	Component->RegisterComponent();
	return Component;
}

// Macros to keep SetupPlayerInputComponent easier to read.
#define AddAxisKeyMapping(Name, Key, Scale) \
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(#Name, Key, Scale)); \
	PlayerInputComponent->BindAxis(#Name, this, &AUEStressPawn::Name);

#define AddActionKeyMapping(Name, Key) \
	UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(#Name, Key)); \
	PlayerInputComponent->BindAction(#Name, IE_Pressed, this, &AUEStressPawn::Name);

void AUEStressPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AddAxisKeyMapping(MoveForwardBackward, EKeys::W, 1.f);
	AddAxisKeyMapping(MoveForwardBackward, EKeys::S, -1.f);
	AddAxisKeyMapping(MoveLeftRight, EKeys::A, -1.f);
	AddAxisKeyMapping(MoveLeftRight, EKeys::D, 1.f);
	AddAxisKeyMapping(MoveUpDown, EKeys::Q, -1.f);
	AddAxisKeyMapping(MoveUpDown, EKeys::E, 1.f);
	AddAxisKeyMapping(LookLeftRight, EKeys::MouseX, 1.f);
	AddAxisKeyMapping(LookUpDown, EKeys::MouseY, 1.f);
	AddAxisKeyMapping(UpdateSpeed, EKeys::MouseWheelAxis, 1.f);

	AddActionKeyMapping(Build, EKeys::B);
	AddActionKeyMapping(ToggleInfo, EKeys::I);
	AddActionKeyMapping(ToggleNaniteStats, EKeys::N);
	AddActionKeyMapping(ToggleLumen, EKeys::L);
	AddActionKeyMapping(ProfileGPU, EKeys::G);
}

void AUEStressPawn::MoveForwardBackward(float Value)
{
	AddMovementInput(GetActorRotation().RotateVector(FVector::ForwardVector), Value);
}

void AUEStressPawn::MoveLeftRight(float Value)
{
	AddMovementInput(GetActorRotation().RotateVector(FVector::RightVector), Value);
}

void AUEStressPawn::MoveUpDown(float Value)
{
	AddMovementInput(GetActorRotation().RotateVector(FVector::UpVector), Value);
}

void AUEStressPawn::LookLeftRight(float Value)
{
	AddControllerYawInput(Value);
}

void AUEStressPawn::LookUpDown(float Value)
{
	AddControllerPitchInput(-Value);
}

void AUEStressPawn::UpdateSpeed(float Value)
{
	if (Value > 0.f)
		MovementComponent->MaxSpeed *= 2.f;
	else if (Value < 0.f)
		MovementComponent->MaxSpeed /= 2.f;

	if (MovementComponent->MaxSpeed < 2.f)
		MovementComponent->MaxSpeed = 2.f;
}

void AUEStressPawn::Build()
{
	MeshCountTarget += MeshCountPerBuild;
}

void AUEStressPawn::ToggleInfo()
{
	InfoState = 1 - InfoState;
	RunCommand(TEXT("stat fps"));
	RunCommand(TEXT("stat unit"));
}

void AUEStressPawn::ToggleNaniteStats()
{
	NaniteStatsState = 1 - NaniteStatsState;
	RunCommand(FString::Printf(TEXT("r.Nanite.ShowStats %d"), NaniteStatsState));
}

void AUEStressPawn::ToggleLumen()
{
	LumenState = 1 - LumenState;
	RunCommand(FString::Printf(TEXT("r.DynamicGlobalIlluminationMethod %d"), LumenState));
	RunCommand(FString::Printf(TEXT("r.ReflectionMethod %d"), LumenState));
}

void AUEStressPawn::ProfileGPU()
{
	RunCommand(TEXT("ProfileGPU"));
}

void AUEStressPawn::RunCommand(const FString& Command)
{
	Cast<APlayerController>(GetController())->ConsoleCommand(Command);
}