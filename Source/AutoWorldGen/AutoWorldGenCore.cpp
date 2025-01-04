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

	bSaveBiomes = false;
	bLoadBiomes = false;

	bAutoGenerate = false;
	bOptimalWorldSize = false;
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
	if (bSaveBiomes)
	{
		bSaveBiomes = false;
		SaveBiomesToJson(FPaths::ProjectContentDir() + TEXT("Biomes.json"));
	}
    if (bLoadBiomes)
    {
        bLoadBiomes = false;
		LoadBiomesFromJson(FPaths::ProjectContentDir() + TEXT("Biomes.json"));
    }

    if (!bAutoGenerate)
    {
        return;
    }

    if (bOptimalWorldSize)
    {
		WorldSize = 8129;
    }

	if (!bIsChanged())
	{
		return;
	}

	VMatrix Heights = GenerateTerrainNoiseMap();

	CreateLandscape(Heights);
}

bool AAutoWorldGenCore::SaveBiomesToJson(const FString& FilePath)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    TArray<TSharedPtr<FJsonValue>> BiomeArray;
    for (const FBiome& Biome : Biomes)
    {
        TSharedPtr<FJsonObject> BiomeObject = MakeShareable(new FJsonObject);

        // Serialize Biome properties
        BiomeObject->SetStringField(TEXT("Name"), Biome.Name);
        BiomeObject->SetNumberField(TEXT("RangeX"), Biome.Range.X);
        BiomeObject->SetNumberField(TEXT("RangeY"), Biome.Range.Y);
        BiomeObject->SetNumberField(TEXT("Seed"), Biome.Seed);
        BiomeObject->SetNumberField(TEXT("Octaves"), Biome.Octaves);
        BiomeObject->SetNumberField(TEXT("Persistence"), Biome.Persistence);
        BiomeObject->SetNumberField(TEXT("Lacunarity"), Biome.Lacunarity);
        BiomeObject->SetNumberField(TEXT("NoiseScale"), Biome.NoiseScale);
        BiomeObject->SetNumberField(TEXT("a"), Biome.a);
        BiomeObject->SetNumberField(TEXT("s"), Biome.s);
        BiomeObject->SetNumberField(TEXT("k"), Biome.k);
        BiomeObject->SetNumberField(TEXT("OriginX"), Biome.Origin.X);
        BiomeObject->SetNumberField(TEXT("OriginY"), Biome.Origin.Y);

        // Add BiomeObject to BiomeArray
        BiomeArray.Add(MakeShareable(new FJsonValueObject(BiomeObject)));
    }

    RootObject->SetArrayField(TEXT("Biomes"), BiomeArray);

    // Serialize RootObject to a JSON formatted string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    if (FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
    {
        // Save JSON string to file
        return FFileHelper::SaveStringToFile(OutputString, *FilePath);
    }

    return false;
}

bool AAutoWorldGenCore::LoadBiomesFromJson(const FString& FilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
    {
        TArray<TSharedPtr<FJsonValue>> BiomeArray = RootObject->GetArrayField(TEXT("Biomes"));
        Biomes.Empty();
        for (TSharedPtr<FJsonValue> Value : BiomeArray)
        {
            TSharedPtr<FJsonObject> BiomeObject = Value->AsObject();
            if (BiomeObject.IsValid())
            {
                FBiome Biome;

                // Deserialize Biome properties
                Biome.Name = BiomeObject->GetStringField(TEXT("Name"));
                Biome.Range.X = BiomeObject->GetNumberField(TEXT("RangeX"));
                Biome.Range.Y = BiomeObject->GetNumberField(TEXT("RangeY"));
                Biome.Seed = (int32)BiomeObject->GetNumberField(TEXT("Seed"));
                Biome.Octaves = (uint8)BiomeObject->GetNumberField(TEXT("Octaves"));
                Biome.Persistence = BiomeObject->GetNumberField(TEXT("Persistence"));
                Biome.Lacunarity = BiomeObject->GetNumberField(TEXT("Lacunarity"));
                Biome.NoiseScale = BiomeObject->GetNumberField(TEXT("NoiseScale"));
                Biome.a = BiomeObject->GetNumberField(TEXT("a"));
                Biome.s = BiomeObject->GetNumberField(TEXT("s"));
                Biome.k = BiomeObject->GetNumberField(TEXT("k"));
                Biome.Origin.X = BiomeObject->GetNumberField(TEXT("OriginX"));
                Biome.Origin.Y = BiomeObject->GetNumberField(TEXT("OriginY"));

                Biomes.Add(Biome);
            }
        }

        return true;
    }

    return false;
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
        BiomeDistances[i] = Subtract(BiomeDistances[i], BiomeDistances[i - 1]);
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

void AAutoWorldGenCore::CreateLandscape(const VMatrix& Heights)
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
	int32 QuadsPerSection;
	int32 SectionsPerComponent;
	if (bOptimalWorldSize)
	{
		QuadsPerSection = 127;
		SectionsPerComponent = 2; // 2x2
	}
    else
	{
		QuadsPerSection = 63;
		SectionsPerComponent = 1;
	}
    const float Scale = TileSize;
    const int32 ComponentSizeQuads = QuadsPerSection * SectionsPerComponent;

    // Calculate the number of components
    const int32 NumComponentsX = FMath::FloorToInt(static_cast<float>(TotalCols - 1) / ComponentSizeQuads);
    const int32 NumComponentsY = FMath::FloorToInt(static_cast<float>(TotalRows - 1) / ComponentSizeQuads);
    
    const int32 Size = NumComponentsX * ComponentSizeQuads;
	const int32 HalfSize = Size / 2;
    const FVector2D LandscapeLocation = FVector2D::ZeroVector;

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

    for (int32 y = 0; y < HeightmapSizeY; y++)
    {
        for (int32 x = 0; x < HeightmapSizeX; x++)
        {
            float HeightValue = Heights[y][x];
            const uint16 HeightUint16 = FMath::Clamp(static_cast<int32>(HeightValue * 128.0f + 32768.0f), 0, 65535);
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
        -HalfSize,
        -HalfSize,
        Size - HalfSize,
        Size - HalfSize,
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
