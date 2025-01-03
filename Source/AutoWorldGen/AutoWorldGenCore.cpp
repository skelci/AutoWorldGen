#include "AutoWorldGenCore.h"

#include "LandscapeStreamingProxy.h"
#include "LandscapeInfo.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "FileHelpers.h"
#include "UObject/SavePackage.h"

AAutoWorldGenCore::AAutoWorldGenCore()
{
	PrimaryActorTick.bCanEverTick = false;

	bAutoGenerate = false;
	WorldSize = 512;
	TileSize = 128;
	Biomes = TArray<FBiome>();

	CurrentWorldSize = 0;
	CurrentTileSize = 0;
	CurrentBiomes = TArray<FBiome>();

	GeneratedLandscape = nullptr;
}

void AAutoWorldGenCore::OnConstruction(const FTransform& Transform)
{
    if (!bAutoGenerate)
    {
        return;
    }

	if (!bIsChanged())
	{
		return;
	}

	VMatrix Heights = GenerateTerrainNoiseMap();

	CreateChunksFromHeights(Heights);
}

bool AAutoWorldGenCore::bIsChanged()
{
	if (WorldSize == CurrentWorldSize &&
        TileSize == CurrentTileSize &&
        Biomes == CurrentBiomes)
    {
        return false;
    }

    if (Biomes.Num() == 0)
    {
        return false;
    }

	CurrentWorldSize = WorldSize;
    CurrentTileSize = TileSize;
    CurrentBiomes = Biomes;

	return true;
}

VMatrix AAutoWorldGenCore::GenerateTerrainNoiseMap()
{
    TArray<VMatrix> BiomeNoiseMaps;
    uint8 BiomeNum = Biomes.Num();
    BiomeNoiseMaps.SetNum(BiomeNum);

    for (uint8 i = 0; i < BiomeNum; i++)
    {
        const FBiome& Biome = Biomes[i];
        BiomeNoiseMaps[i] = GetNoiseMap(
            WorldSize,
            Biome.Range,
            Biome.Seed,
            Biome.Octaves,
            Biome.Persistence,
            Biome.Lacunarity,
			Biome.NoiseScale
        );
    }

    TArray<VMatrix> BiomeDistances;
    BiomeDistances.SetNum(BiomeNum);
    for (uint8 i = 0; i < BiomeNum; i++)
    {
        const FBiome& Biome = Biomes[i];
        BiomeDistances[i] = GetDistancesFromCenter(
            WorldSize,
			FVector2D(Biome.Origin.X + WorldSize / 2, Biome.Origin.Y + WorldSize / 2)
        );

        BiomeDistances[i] = Subtract(1, Fade(BiomeDistances[i], Biome.a, Biome.s, Biome.k));
    }

    if (BiomeNum > 1)
    {
        BiomeDistances[BiomeNum - 1] = Subtract(1, BiomeDistances[BiomeNum - 2]);
    }
    for (int8 i = BiomeNum - 2; i > 0; i--)
    {
        BiomeDistances[i] = Subtract(1, BiomeDistances[i - 1]);
    }

    for (uint8 i = 0; i < BiomeNum; i++)
    {
        BiomeNoiseMaps[i] = Multiply(BiomeNoiseMaps[i], BiomeDistances[i]);
    }

    VMatrix Heights = BiomeNoiseMaps[0];
    for (uint8 i = 1; i < BiomeNum; i++)
    {
        Heights = Add(Heights, BiomeNoiseMaps[i]);
    }

	return Heights;
}

void AAutoWorldGenCore::CreateChunksFromHeights(const VMatrix& Heights)
{
#if WITH_EDITOR
    // Delete existing landscape if it exists  
    if (GeneratedLandscape && !GeneratedLandscape->IsPendingKillPending())
    {
        GetWorld()->DestroyActor(GeneratedLandscape);
        GeneratedLandscape = nullptr;
    }

    // Early exit if Heights is empty
    const int32 TotalRows = Heights.Num();
    if (TotalRows == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Heights array is empty."));
        return;
    }
    const int32 TotalCols = Heights[0].Num();
    if (TotalCols == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Heights[0] array is empty."));
        return;
    }

    // Parameters for landscape
    const float Scale = TileSize;
    const int32 QuadsPerSection = 63;
    const int32 SectionsPerComponent = 1;
    const int32 ComponentSizeQuads = QuadsPerSection * SectionsPerComponent;

    // Calculate the number of components
    const int32 NumComponentsX = FMath::CeilToInt(static_cast<float>(TotalCols - 1) / ComponentSizeQuads);
    const int32 NumComponentsY = FMath::CeilToInt(static_cast<float>(TotalRows - 1) / ComponentSizeQuads);
    
    const int32 Size = NumComponentsX * ComponentSizeQuads - 1;
	const int32 HalfSize = Size / 2;
    const FVector2D LandscapeLocation = FVector2D(-HalfSize * Scale);

    // Ensure heightmap size matches required size
    const int32 HeightmapSizeX = NumComponentsX * ComponentSizeQuads + 1;
    const int32 HeightmapSizeY = NumComponentsY * ComponentSizeQuads + 1;

    // Create the main Landscape Actor
    GeneratedLandscape = GetWorld()->SpawnActor<ALandscape>();
    GeneratedLandscape->SetActorLocation(FVector(LandscapeLocation.X, LandscapeLocation.Y, 0));
    GeneratedLandscape->SetActorScale3D(FVector(Scale));

    // Prepare height data
    TArray<uint16> HeightData;
    HeightData.SetNumZeroed(HeightmapSizeX * HeightmapSizeY);

    for (int32 y = 0; y < HeightmapSizeY; ++y)
    {
        for (int32 x = 0; x < HeightmapSizeX; ++x)
        {
            uint16 HeightUint16 = 32768; // Default flat height
            if (y < TotalRows && x < TotalCols)
            {
                float HeightValue = Heights[y][x];
                HeightUint16 = FMath::Clamp(static_cast<int32>(HeightValue * 128.0f + 32768.0f), 0, 65535);
            }
            HeightData[y * HeightmapSizeX + x] = HeightUint16;
        }
    }

    // Generate a new GUID for the landscape
    FGuid LandscapeGuid = FGuid::NewGuid();
    GeneratedLandscape->SetLandscapeGuid(LandscapeGuid);

    // Prepare HeightMapData with default FGuid() key
    TMap<FGuid, TArray<uint16>> HeightMapData;
    HeightMapData.Add(FGuid(), HeightData); // Use default FGuid() as key

    // Prepare MaterialLayerMap
    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerMap;

    // Create layer info with default FGuid()
    FLandscapeImportLayerInfo LayerInfo;
    LayerInfo.LayerName = FName("Layer_0"); // You can set an appropriate name
    LayerInfo.LayerData.SetNumZeroed(HeightmapSizeX * HeightmapSizeY);

    // Add the layer info to MaterialLayerMap with default FGuid() key
    MaterialLayerMap.Add(FGuid(), { LayerInfo });

    TArray<FLandscapeLayer> NoImportLayers;

    // Call the Import method
    GeneratedLandscape->Import(
        LandscapeGuid,
        0,
        0,
        Size,
        Size,
        SectionsPerComponent,
        QuadsPerSection,
        HeightMapData,
        TEXT("GeneratedHeightmap"),
        MaterialLayerMap,
        ELandscapeImportAlphamapType::Layered,
        TArrayView<const FLandscapeLayer>(NoImportLayers)
    );

    GeneratedLandscape->PostEditChange();
#endif
}

VMatrix AAutoWorldGenCore::GetNoiseMap(
    const uint16 Size,
    const FVector2D Range,
    int32 Seed,
    const uint8 Octaves,
    const double Persistence,
    const double Lacunarity,
    const double NoiseScale)
{
    VMatrix NoiseMap;
    NoiseMap.SetNum(Size);

	float MaxNoiseHeight = 0.0f;
	TArray<FVector2D> OctaveOffsets;
	OctaveOffsets.SetNum(Octaves);
	for (uint8 o = 0; o < Octaves; ++o)
	{
		MaxNoiseHeight += FMath::Pow(Persistence, o);
        
        FRandomStream RandomStream(Seed++);
        float OffsetX = RandomStream.FRandRange(-100000.0f, 100000.0f);
        float OffsetY = RandomStream.FRandRange(-100000.0f, 100000.0f);
		OctaveOffsets[o] = FVector2D(OffsetX, OffsetY);
	}

    // Loop through each point in the noise map
    for (uint16 y = 0; y < Size; ++y)
    {
        NoiseMap[y].SetNum(Size);
        for (uint16 x = 0; x < Size; ++x)
        {
            float Amplitude = 1.0f;
            float FrequencyAcc = 1.0f;
            float NoiseHeight = 0.0f;

            // Apply multiple octaves
            for (uint8 o = 0; o < Octaves; ++o)
            {
                // Generate sample points
                float SampleX = x * FrequencyAcc * NoiseScale + OctaveOffsets[o].X;
                float SampleY = y * FrequencyAcc * NoiseScale + OctaveOffsets[o].Y;

                // Get Perlin noise value (returns value in range [-1, 1])
                float PerlinValue = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY)) * Amplitude;

                NoiseHeight += PerlinValue;

                Amplitude *= Persistence;
                FrequencyAcc *= Lacunarity;
            }

            // Normalize the noise value to the specified range
            float NormalizedHeight = FMath::GetMappedRangeValueClamped(
                FVector2D(-1.0f, 1.0f),
                Range,
				NoiseHeight / MaxNoiseHeight
            );

            NoiseMap[y][x] = NormalizedHeight;
        }
    }

    return NoiseMap;
}

VMatrix AAutoWorldGenCore::GetDistancesFromCenter(const uint16 Size, FVector2D Origin)
{
	VMatrix Distances;

	Distances.SetNum(Size);
	for (uint16 y = 0; y < Size; ++y)
	{
		Distances[y].SetNum(Size);
		for (uint16 x = 0; x < Size; ++x)
		{
			FVector2D Point(x, y);
			float Distance = FVector2D::Distance(Point, Origin);
			Distances[y][x] = Distance;
		}
	}

	return Distances;
}
