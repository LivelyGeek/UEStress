// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UEStressPawn.generated.h"

UCLASS()
class UESTRESS_API AUEStressPawn : public APawn
{
	GENERATED_BODY()

public:
	AUEStressPawn();

	// See UEStressPawn.cpp for comments and to configure values in constructor.
	virtual void Tick(float DeltaTime) override;
	class AActor* AddMeshActor(int32 MeshIndex);
	template<class T> T* AddStaticMeshComponent(class AActor* Actor, int32 MeshIndex);

	class UCameraComponent* CameraComponent;
	class UFloatingPawnMovement* MovementComponent;
	int32 MeshCount;
	int32 MeshCountTarget;
	int32 MeshCountPerBuild;
	int32 MeshCountPerTick;
	float MeshSpacing;
	FVector MeshLocation;
	int32 MeshY;
	int32 MeshYMax;
	int32 MeshZ;
	int32 MeshZMax;
	TArray<class UStaticMesh*> Meshes;
	TArray<class AActor*> MeshActors;
	class UMaterial* MeshMaterial;
	bool bUseInstanced;
	int32 ActorsPerMesh;

	// Bind input for movement and actions.
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void MoveForwardBackward(float Value);
	void MoveLeftRight(float Value);
	void MoveUpDown(float Value);
	void LookLeftRight(float Value);
	void LookUpDown(float Value);
	void UpdateSpeed(float Value);
	void Build();
	void ToggleInfo();
	void ToggleNaniteStats();
	void ToggleLumen();
	void ProfileGPU();
	void RunCommand(const FString& Command);

protected:
	virtual void BeginPlay() override;
};