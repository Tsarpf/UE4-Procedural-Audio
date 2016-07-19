// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "FrequencyFinder.h"
#include "RuntimeMeshComponent.h"
#include "ProceduralSoundWave.generated.h"

UCLASS()
class PROCEDURALMESHDEMOS_API AProceduralSoundWave : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProceduralSoundWave();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif   // WITH_EDITOR

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	FVector Size = FVector(300.0f, 200.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	UMaterialInterface* Material;

protected:
	UPROPERTY()
	URuntimeMeshComponent* MeshComponent;

private:

	// Create vertex and triangle buffers
	void SetupMeshBuffers();
	
	// Add mesh components, bounding box, etc.
	void GenerateMesh(TArray<float>&);

	// Define vertices
	void GenerateWave(TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles, FVector Size, TArray<float>& Heights, TArray<float>& PreviousHeights);

	// Actually add a quad to vertex and index (triangle) lists
	void BuildQuad(
		TArray<FRuntimeMeshVertexSimple>& Vertices,
		TArray<int32>& Triangles,
		FVector BottomLeft,
		FVector BottomRight,
		FVector TopRight,
		FVector TopLeft,
		int32& VertexOffset,
		int32& TriangleOffset);

	bool m_buffersInitialized = false;
	TArray<FRuntimeMeshVertexSimple> m_vertices;
	TArray<int32> m_triangles;

	int32 m_vertexIdx = 0;
	int32 m_triangleIdx = 0;
	float m_xWorldPos = 0;
	int32 m_vertexCount;
	float m_xStepSize = 10;
	float m_yStepSize = 10;
	float m_timer = 0;
	//uint32 m_zStepSize = 1; //set by scaling the height algorithm

	FrequencyFinder m_freqFinder;

	TArray<float> m_previousHeights;
	TArray<float> m_zeroHeights;
	bool m_first = true;
};
