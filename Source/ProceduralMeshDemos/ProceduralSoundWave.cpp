// Fill out your copyright notice in the Description page of Project Settings.

#include "ProceduralMeshDemos.h"
#include "ProceduralSoundWave.h"
#include "EngineUtils.h"

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
void PrintError(wchar_t* string);

// Called when the game starts or when spawned
void AProceduralSoundWave::BeginPlay()
{
	Super::BeginPlay();

	m_freqFinder.Start();

	for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		FString name = (*ActorItr)->GetName();
		UE_LOG(LogTemp, Warning, TEXT("MyCharacter's Name is %s"), *name);
		if (name == "vittuspawnaamuthomo_C_0")
		{
			UE_LOG(LogTemp, Warning, TEXT("found it motherFucker"));
			m_cameraActor = (*ActorItr);
		}
	}

	GenerateMesh(m_zeroHeights);
}


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
		//PrintError(TEXT("heights was 0"));
		//return GenerateMesh(m_zeroHeights);
	}
	while (heights.Num() > 0)
	{
		// Move camera
		//FVector location = GetOwner()->GetRootComponent()->GetOwner()->GetActorLocation();
		FVector location = m_cameraActor->GetActorLocation();
		location.X += m_xStepSize;
		m_cameraActor->SetActorLocation(location);

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

		//m_xWorldPos = 0;
	}

	// ratio of being there y / (Size.Y * m_yStepSize)

	// scaled to other thing...
	// heights.size() * above ratio


	float idxMax = (float)std::min(Heights.Num(), PreviousHeights.Num());
	auto time = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	float epsilon = 0.1;
	int lowFreqBinThreshold = std::min((int)idxMax, 20);
	float yPos = 0;
	float idx;
	for (idx = 0; idx < Size.Y - 2; idx++)
	{
		//int repeats = FMath::Loge(idxMax - idx);
		//if (idx < 10) repeats *= 2;
		int repeats = 20;
		for (int y = 0; y < repeats; y++)
		{
			int scale = 16;
			yPos += m_yStepSize;

			/*
			int heightIdx, heightIdxNext;
				heightIdx = idx;
				heightIdxNext
					= y + 1 < repeats
					? idx
					: idx + 1; //+ 1;
			*/
			int heightIdx, heightIdxNext;
			if (idx < lowFreqBinThreshold)
			{
				heightIdx = idx;
				heightIdxNext
					= y + 1 < repeats
					? idx
					: idx + 1; //+ 1;
			}
			else
			{
				heightIdx = static_cast<int>(idx / Size.Y * idxMax);
				heightIdxNext
					= y + 1 < repeats
					? heightIdx
					: static_cast<int>((idx + 1) / Size.Y * idxMax);
			}
			float bottomLeftZ = 0, bottomRightZ = 0, topLeftZ = 0, topRightZ = 0;
			float count = heightIdx == heightIdxNext ? 1 : heightIdxNext - heightIdx;
			for (int i = 0; i < heightIdxNext - heightIdx; i++)
			{
				bottomLeftZ += PreviousHeights[heightIdx + i];
				bottomRightZ += PreviousHeights[heightIdxNext + i];
				topLeftZ += Heights[heightIdx + i];
				topRightZ += Heights[heightIdxNext + i];
			}
			bottomLeftZ /= count;
			bottomLeftZ *= scale;
			//UE_LOG(LogTemp, Warning, TEXT("bottomleftz %f"), bottomLeftZ);
			bottomRightZ /= count;
			bottomRightZ *= scale;
			topLeftZ /= count;
			topLeftZ *= scale;
			topRightZ /= count;
			topRightZ *= scale;
			FVector bottomLeft(m_xWorldPos, yPos, bottomLeftZ);
			FVector bottomRight(m_xWorldPos, yPos + m_yStepSize, bottomRightZ);
			FVector topLeft(m_xWorldPos + m_xStepSize, yPos, topLeftZ);
			FVector topRight(m_xWorldPos + m_xStepSize, yPos + m_yStepSize, topRightZ);
			BuildQuad(m_vertices, m_triangles, bottomLeft, bottomRight, topRight, topLeft, m_vertexIdx, m_triangleIdx);
		}
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

