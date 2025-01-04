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
	FVector2D Range = FVector2D(-16.0, 16.0);  
						  
	UPROPERTY(EditAnywhere, Category = "Noise")
	int32 Seed = 0;			  
						  
	UPROPERTY(EditAnywhere, Category = "Noise")
	uint8 Octaves = 1; 
						  
	UPROPERTY(EditAnywhere, Category = "Noise", meta = (ClampMin = "0"))
	double Persistence = 0.5;	  
						  
	UPROPERTY(EditAnywhere, Category = "Noise", meta = (ClampMin = "0"))
	double Lacunarity = 2;

	UPROPERTY(EditAnywhere, Category = "Noise", meta = (ClampMin = "0.00000000001"))
	double NoiseScale = 0.1;

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
			&& Octaves == Other.Octaves
			&& FMath::IsNearlyEqual(Persistence, Other.Persistence)
			&& FMath::IsNearlyEqual(Lacunarity, Other.Lacunarity)
			&& FMath::IsNearlyEqual(NoiseScale, Other.NoiseScale)
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

	// This will override the WorldSize, SectionsPerComponent, and QuadsPerSection
	UPROPERTY(EditAnywhere, Category = "AutoWorldGen")
	bool bOptimalWorldSize;

	UPROPERTY(EditAnywhere, Category = "AutoWorldGen", meta = (ClampMin = "64"))
	uint16 WorldSize;

	UPROPERTY(EditAnywhere, Category = "AutoWorldGen", meta = (ClampMin = "4"))
	uint8 TileSize;

	UPROPERTY(EditAnywhere, Category = "AutoWorldGen")
	TArray<FBiome> Biomes;

private:
	uint16 CurrentWorldSize;
	uint8 CurrentTileSize;
	TArray<FBiome> CurrentBiomes;

	UPROPERTY(VisibleAnywhere, Transient)
	ALandscape* GeneratedLandscape;

	bool bIsChanged();

	VMatrix GenerateTerrainNoiseMap();

	void CreateLandscape(const VMatrix& Heights);

	VMatrix GetNoiseMap(
		const uint16 Size,
		const FVector2D Range,
		int32 Seed,
		const uint8 Octaves,
		const double Persistence,
		const double Lacunarity,
		const double Scale
	);

	VMatrix GetDistancesFromCenter(const uint16 Size, FVector2D Origin);
};
