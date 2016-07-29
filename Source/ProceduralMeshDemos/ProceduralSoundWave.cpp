// Fill out your copyright notice in the Description page of Project Settings.

#include "ProceduralMeshDemos.h"
#include "ProceduralSoundWave.h"


// Sets default values
AProceduralSoundWave::AProceduralSoundWave()
{
	m_previousHeights.AddZeroed(1024);
	//m_zeroHeights.AddZeroed(1024);
	for (int i = 0; i < 1024; i++)
	{
		m_zeroHeights.Add(1);
	}
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Needs some root component I guess?
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	MeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("ProceduralMesh"));
	// I wonder what this does
	MeshComponent->bShouldSerializeMeshData = false;
	MeshComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AProceduralSoundWave::BeginPlay()
{
	Super::BeginPlay();

	m_freqFinder.Start();

	GenerateMesh(m_zeroHeights);
}

void PrintError(wchar_t* string);

// Called every frame
void AProceduralSoundWave::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	int newTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	if (newTime > m_timer)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Some variable values: x: %d"), newTime));
		m_timer++;
	}


	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Some variable values: x: %f"), heights[0]));
	int timeMS = UGameplayStatics::GetRealTimeSeconds(GetWorld()) * 1000.0f;
	auto heights = m_freqFinder.GetHeights();
	if (heights.Num() == 0)
	{
		PrintError(TEXT("heights was 0"));
		//return GenerateMesh(m_zeroHeights);
	}
	while (heights.Num() > 0)
	{
		GenerateMesh(heights);
		heights = m_freqFinder.GetHeights();
	}
}

#if WITH_EDITOR
void AProceduralSoundWave::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateMesh(m_previousHeights);
	GenerateMesh(m_previousHeights);
}
#endif // WITH_EDITOR


void AProceduralSoundWave::SetupMeshBuffers()
{
	m_vertexCount = Size.X * Size.Y * 6; // 6 Vertices per quad (ie. per square "meter" or whatever)
	m_vertices.AddUninitialized(m_vertexCount);
	m_triangles.AddUninitialized(Size.X * Size.Y * 6); // 6 triangle points per quad
}

void AProceduralSoundWave::GenerateMesh(TArray<float>& Heights)
{
	if (!m_buffersInitialized)
	{
		SetupMeshBuffers();
		m_buffersInitialized = true;
	}

	//FBox boundingBox = FBox(-Size / 2.0f, Size / 2.0f);
	//FBox boundingBox = FBox(FVector(m_xWorldPos - Size.X * m_xStepSize, m_yStepSize * Size.Y, -100 ;

	GenerateWave(m_vertices, m_triangles, Size, Heights, m_previousHeights);

	m_previousHeights = Heights;

	MeshComponent->ClearAllMeshSections();
	//MeshComponent->CreateMeshSection(0, m_vertices, m_triangles, boundingBox, false, EUpdateFrequency::Infrequent);
	MeshComponent->CreateMeshSection(0, m_vertices, m_triangles, false, EUpdateFrequency::Infrequent);
	MeshComponent->SetMaterial(0, Material);
}

// Generate one slice of the wave going forward
// In UE4, positive X is "forward", positive Y is "right" and positive Z is "up".
void AProceduralSoundWave::GenerateWave
(
	TArray<FRuntimeMeshVertexSimple>& Vertices,
	TArray<int32>& Triangles,
	FVector Size,
	TArray<float>& Heights,
	TArray<float>& PreviousHeights
)
{
	// We need to keep track where in the vertex array we're going	
	// WorldPos keeps growing, array pos loops
	if (m_vertexIdx + Size.X * Size.Y >= m_vertexCount)
	{
		m_vertexIdx = 0;
		// We should assert that triangles have gone a full circle as well
		m_triangleIdx = 0;

		m_xWorldPos = 0;
	}

	// ratio of being there y / (Size.Y * m_yStepSize)

	// scaled to other thing...
	// heights.size() * above ratio

	float smallerSize = (float)std::min(Heights.Num(), PreviousHeights.Num());
	auto time = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	float epsilon = 0.1;
	for (float y = 1; y < Size.Y - 1 - epsilon; y++)
	//for (float y = 0; y < Size.Y; y++)
	{
		float yPos = y * m_yStepSize;
		int heightIdx = static_cast<int>(y / Size.Y * smallerSize);
		int heightIdxNext = static_cast<int>((y + 1) / Size.Y * smallerSize);
		//int heightIdxNext = heightIdx + 1;
		int scale = 16;
		float bottomLeftZ = PreviousHeights[heightIdx] * scale;
		float bottomRightZ = PreviousHeights[heightIdxNext] * scale;
		float topLeftZ = Heights[heightIdx] * scale;
		float topRightZ = Heights[heightIdxNext] * scale;
		FVector bottomLeft(m_xWorldPos, yPos, bottomLeftZ);
		FVector bottomRight(m_xWorldPos, yPos + m_yStepSize, bottomRightZ);
		FVector topLeft(m_xWorldPos + m_xStepSize, yPos, topLeftZ);
		FVector topRight(m_xWorldPos + m_xStepSize, yPos + m_yStepSize, topRightZ);

		/*
		float zBottomLeft = 
			sin(m_xWorldPos / 100.0f) * 100.0f
			//+ cos(y / 100.0f + time * 4.f)
			//cos(y / 100.0f + time * 4.f)
			+ cos(y / 100.0f + time * 4.f)
			* 100.f;
		float zBottomRight = 
			sin(m_xWorldPos / 100.0f) * 100.0f
			//+ cos(y / 100.0f + time * 4.f)
			//cos(y / 100.0f + time * 4.f)
			+ cos((y + m_yStepSize) / 100.0f + time * 4.f)
			* 100.f;

		float zTopLeft = 
			sin((m_xWorldPos + m_xStepSize) / 100.0f) * 100.0f
			//+ cos((y + m_yStepSize) / 100.0f  + time * 4.f)
			//cos((y + m_yStepSize) / 100.0f  + time * 4.f)
			+ cos(y / 100.0f + time * 4.f)
			* 100.f;
		float zTopRight = 
			sin((m_xWorldPos + m_xStepSize) / 100.0f) * 100.0f
			//+ cos((y + m_yStepSize) / 100.0f  + time * 4.f)
			//cos((y + m_yStepSize) / 100.0f  + time * 4.f)
			+ cos((y + m_yStepSize) / 100.0f + time * 4.f)
			* 100.f;
		FVector bottomLeft(m_xWorldPos, y, zBottomLeft);
		FVector bottomRight(m_xWorldPos, y + m_yStepSize, zBottomRight);
		FVector topRight(m_xWorldPos + m_xStepSize, y + m_yStepSize, zTopRight);
		FVector topLeft(m_xWorldPos + m_xStepSize, y, zTopLeft);
		*/


		BuildQuad(m_vertices, m_triangles, bottomLeft, bottomRight, topRight, topLeft, m_vertexIdx, m_triangleIdx);
	}

	//m_arrayPos++;
	m_xWorldPos += m_xStepSize;
}

void GenerateNormals()
{

}

void AddTriangle()
{

}

void AProceduralSoundWave::BuildQuad(
	TArray<FRuntimeMeshVertexSimple>& Vertices,
	TArray<int32>& Triangles,
	FVector BottomLeft,
	FVector BottomRight,
	FVector TopRight,
	FVector TopLeft,
	int32& VertexOffset,
	int32& TriangleOffset)
{
	int32 Index1 = VertexOffset++;
	int32 Index2 = VertexOffset++;
	int32 Index3 = VertexOffset++;
	int32 Index4 = VertexOffset++;
	int32 Index5 = VertexOffset++;
	int32 Index6 = VertexOffset++;
	Vertices[Index1].Position = BottomLeft;
	Vertices[Index2].Position = BottomRight;
	Vertices[Index3].Position = TopRight;
	Vertices[Index4].Position = TopLeft;
	Vertices[Index5].Position = BottomLeft;
	Vertices[Index6].Position = TopRight;

	Vertices[Index1].UV0 = FVector2D(0.0f, 1.0f);
	Vertices[Index2].UV0 = FVector2D(1.0f, 1.0f);
	Vertices[Index3].UV0 = FVector2D(1.0f, 0.0f);

	Vertices[Index4].UV0 = FVector2D(0.0f, 0.0f);
	Vertices[Index5].UV0 = FVector2D(0.0f, 1.0f);
	Vertices[Index6].UV0 = FVector2D(1.0f, 0.0f);

	// Triangle 1
	Triangles[TriangleOffset++] = Index1;
	Triangles[TriangleOffset++] = Index2;
	Triangles[TriangleOffset++] = Index3;
	/*
	FVector one(Vertices[Index1].Position + Vertices[Index2].Position);
	one.Normalize();
	FVector two(Vertices[Index1].Position + Vertices[Index3].Position);
	two.Normalize();
	FPackedNormal normal1 = -FVector::CrossProduct(one, two);
	Vertices[Index1].Normal = normal1;
	Vertices[Index1].Tangent = one;
	Vertices[Index2].Normal = normal1;
	Vertices[Index2].Tangent = one;
	Vertices[Index3].Normal = normal1;
	Vertices[Index3].Tangent = one;
	*/

	// Triangle 2
	Triangles[TriangleOffset++] = Index4;
	Triangles[TriangleOffset++] = Index5;
	Triangles[TriangleOffset++] = Index6;
	/*
	FVector three(Vertices[Index4].Position + Vertices[Index5].Position);
	three.Normalize();
	FVector four(Vertices[Index4].Position + Vertices[Index6].Position);
	four.Normalize();
	FPackedNormal normal2 = -FVector::CrossProduct(three, four);
	Vertices[Index4].Normal = normal2;
	Vertices[Index4].Tangent = three;
	Vertices[Index5].Normal = normal2;
	Vertices[Index5].Tangent = three;
	Vertices[Index6].Normal = normal2;
	Vertices[Index6].Tangent = three;
	*/
}

