// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VaribleMatrix.h"
#include "GameFramework/Actor.h"
#include "Landscape.h"

#include "AutoWorldGenCore.generated.h"

using namespace VaribleMatrix;

USTRUCT(BlueprintType)
struct FBiome
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString Name = "Biome";
						  
	UPROPERTY(EditAnywhere, Category = "Noise")
	FVector2D Range = FVector2D(-1.0, 1.0);  
						  
	UPROPERTY(EditAnywhere, Category = "Noise")
	int32 Seed = 0;			  
						  
	UPROPERTY(EditAnywhere, Category = "Noise")
	double Frequency = 1;	  
						  
	UPROPERTY(EditAnywhere, Category = "Noise")
	uint8 Octaves = 1; 
						  
	UPROPERTY(EditAnywhere, Category = "Noise")
	double Persistence = 0.5;	  
						  
	UPROPERTY(EditAnywhere, Category = "Noise")
	double Lacunarity = 0.5;

	UPROPERTY(EditAnywhere, Category = "Fade")
	double a = 2;

	UPROPERTY(EditAnywhere, Category = "Fade")
	double s = 0;

	UPROPERTY(EditAnywhere, Category = "Fade")
	double k = 1;

	UPROPERTY(EditAnywhere, Category = "Fade")
	FVector2D Origin = FVector2D(0, 0);

	bool operator==(const FBiome& Other) const
	{
		return Range == Other.Range
			&& Origin == Other.Origin
			&& Seed == Other.Seed
			&& FMath::IsNearlyEqual(Frequency, Other.Frequency)
			&& Octaves == Other.Octaves
			&& FMath::IsNearlyEqual(Persistence, Other.Persistence)
			&& FMath::IsNearlyEqual(Lacunarity, Other.Lacunarity)
			&& FMath::IsNearlyEqual(a, Other.a)
			&& FMath::IsNearlyEqual(s, Other.s)
			&& FMath::IsNearlyEqual(k, Other.k);
	}
};

UCLASS()
class AUTOWORLDGEN_API AAutoWorldGenCore : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAutoWorldGenCore();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "AutoWorldGen")
	bool bAutoGenerate;

	UPROPERTY(EditAnywhere, Category = "AutoWorldGen")
	uint8 ChunkSize;

	UPROPERTY(EditAnywhere, Category = "AutoWorldGen")
	uint8 ChunksFromCenter;

	UPROPERTY(EditAnywhere, Category = "AutoWorldGen")
	uint8 TileSize;

	UPROPERTY(EditAnywhere, Category = "AutoWorldGen")
	TArray<FBiome> Biomes;

private:
	uint16 WorldSize;

	uint8 CurrentChunkSize;
	uint8 CurrentChunksFromCenter;
	uint8 CurrentTileSize;
	TArray<FBiome> CurrentBiomes;

	UPROPERTY(VisibleAnywhere, Transient)
	ALandscape* GeneratedLandscape;

	bool bIsChanged();

	VMatrix GenerateTerrainNoiseMap();

	void CreateChunksFromHeights(const VMatrix& Heights);

	VMatrix GetNoiseMap(
		const uint16 Size,
		const FVector2D Range,
		const int32 Seed,
		const double Frequency,
		const uint8 Octaves,
		const double Persistence,
		const double Lacunarity
	);

	VMatrix GetDistancesFromCenter(const uint16 Size, FVector2D Origin);
};
