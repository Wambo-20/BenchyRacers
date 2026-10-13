#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BenchyLogo.generated.h"
class UTexture2D;
class UStaticMeshComponent;
UCLASS()
class BENCHYRACERS_API UBenchyLogo : public UObject {
    GENERATED_BODY()
public:
    void Load();
    bool Import(const FString& Path,bool Persist=true);
    bool Save();
    TArray<uint8> NetworkPNG();
    bool ImportNetwork(const TArray<uint8>& PNG);
    void Rebuild();
    void Apply(UStaticMeshComponent* Mesh);
    void Remove();
    void EnableLayers();
    bool AddLayer(const FString& Path);
    void DuplicateLayer();
    void DeleteLayer();
    void MoveLayer(int32 Direction);
    UBenchyLogo* ActiveLayer() const;
    void BuildAtlas(const FVector& BoundsSize);
    bool SaveLayers();
    bool LoadLayers();
    UPROPERTY() TArray<TObjectPtr<UBenchyLogo>> Layers;
    int32 ActiveIndex=0;
    bool Layered=false;
    UPROPERTY() TObjectPtr<UTexture2D> Texture;
    FString Notice,Name;
    bool Enabled=false;
    int32 Side=0,Outline=1,OutlineWidth=3;
    float X=.5f,Y=.25f,Size=.34f,Angle=0;
    int32 PixelWidth=0,PixelHeight=0;
    TArray<FColor> Source,Rendered;
private:
    void CopyLayerTo(UBenchyLogo* Other) const;
    FString AtlasSignature;
    FVector AtlasSize=FVector(90,40,50);
    TArray<uint8> Original;
};
