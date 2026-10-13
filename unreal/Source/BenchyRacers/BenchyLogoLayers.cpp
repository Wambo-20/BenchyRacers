#include "BenchyLogo.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
namespace {FString LayerDir(){return FPaths::ProjectSavedDir()/TEXT("Customization");}}
void UBenchyLogo::CopyLayerTo(UBenchyLogo* O) const {
    O->Name=Name;O->Enabled=Enabled;O->Side=FMath::Clamp(Side,0,4);O->Outline=Outline;O->OutlineWidth=OutlineWidth;O->X=X;O->Y=Y;O->Size=Size;O->Angle=Angle;
    O->Source=Source;O->Original=Original;O->PixelWidth=PixelWidth;O->PixelHeight=PixelHeight;O->Rebuild();
}
UBenchyLogo* UBenchyLogo::ActiveLayer() const {return Layered?(Layers.IsValidIndex(ActiveIndex)?Layers[ActiveIndex].Get():nullptr):const_cast<UBenchyLogo*>(this);}
void UBenchyLogo::EnableLayers(){if(Layered)return;if(!Source.IsEmpty()){auto L=NewObject<UBenchyLogo>(this);CopyLayerTo(L);Layers.Add(L);}Layered=true;Side=5;ActiveIndex=0;AtlasSignature.Reset();}
bool UBenchyLogo::AddLayer(const FString& Path){EnableLayers();if(Layers.Num()>=8){Notice=TEXT("Maximal acht Logo-Ebenen.");return false;}auto L=NewObject<UBenchyLogo>(this);L->Y=.17f;L->Size=.28f;if(!L->Import(Path,false)){Notice=L->Notice;return false;}Layers.Add(L);ActiveIndex=Layers.Num()-1;Notice=TEXT("Neue Logo-Ebene hinzugefügt.");return true;}
void UBenchyLogo::DuplicateLayer(){auto L=ActiveLayer();if(!L||Layers.Num()>=8)return;auto Copy=NewObject<UBenchyLogo>(this);L->CopyLayerTo(Copy);Copy->Name+=TEXT(" · Kopie");Layers.Insert(Copy,ActiveIndex+1);++ActiveIndex;}
void UBenchyLogo::DeleteLayer(){if(!Layers.IsValidIndex(ActiveIndex))return;Layers.RemoveAt(ActiveIndex);ActiveIndex=FMath::Clamp(ActiveIndex,0,FMath::Max(0,Layers.Num()-1));}
void UBenchyLogo::MoveLayer(int32 Direction){int32 Other=ActiveIndex+Direction;if(!Layers.IsValidIndex(ActiveIndex)||!Layers.IsValidIndex(Other))return;Layers.Swap(ActiveIndex,Other);ActiveIndex=Other;}
void UBenchyLogo::BuildAtlas(const FVector& BoundsSize){
    AtlasSize=BoundsSize;FString Signature=BoundsSize.ToString();bool Any=false;
    for(auto L:Layers){Signature+=FString::Printf(TEXT("|%d,%d,%d,%.6f,%.6f,%.6f,%.6f"),L->Texture?L->Texture->GetUniqueID():0,L->Enabled,L->Side,L->X,L->Y,L->Size,L->Angle);Any|=L->Enabled&&!L->Rendered.IsEmpty();}
    Enabled=Any;Side=5;if(Signature==AtlasSignature)return;AtlasSignature=Signature;
    constexpr int32 Tile=512,Dim=1024;Rendered.Init(FColor(0,0,0,0),Dim*Dim);PixelWidth=PixelHeight=Dim;
    for(auto L:Layers){if(!L->Enabled||L->Rendered.IsEmpty())continue;
        for(int32 Face=0;Face<4;++Face){if(!(L->Side==Face||(L->Side==3&&Face<2)||(L->Side==4&&Face==3)))continue;if(L->Side==3&&Face==3)continue;
            float FaceW=Face==3?BoundsSize.Y:BoundsSize.X,FaceH=Face==2?BoundsSize.Y:BoundsSize.Z;
            float CenterU=Face==0?1-L->X:L->X,CenterV=1-L->Y;
            float Width=FMath::Max(.001f,L->Size*FaceW),Height=Width*L->PixelHeight/FMath::Max(1,L->PixelWidth);
            float C=FMath::Cos(FMath::DegreesToRadians(L->Angle)),S=FMath::Sin(FMath::DegreesToRadians(L->Angle));
            float HalfU=(FMath::Abs(C)*Width+FMath::Abs(S)*Height)*.5f/FaceW,HalfV=(FMath::Abs(S)*Width+FMath::Abs(C)*Height)*.5f/FaceH;
            int32 MinU=FMath::Clamp(FMath::FloorToInt((CenterU-HalfU)*Tile),0,Tile-1),MaxU=FMath::Clamp(FMath::CeilToInt((CenterU+HalfU)*Tile),0,Tile-1);
            int32 MinV=FMath::Clamp(FMath::FloorToInt((CenterV-HalfV)*Tile),0,Tile-1),MaxV=FMath::Clamp(FMath::CeilToInt((CenterV+HalfV)*Tile),0,Tile-1);
            for(int32 V=MinV;V<=MaxV;++V)for(int32 U=MinU;U<=MaxU;++U){float DX=((U+.5f)/Tile-CenterU)*FaceW,DY=((V+.5f)/Tile-CenterV)*FaceH;
                float PX=(C*DX+S*DY)/Width+.5f,PY=(-S*DX+C*DY)/Height+.5f;if(PX<0||PX>=1||PY<0||PY>=1)continue;
                FColor Ink=L->Rendered[FMath::Clamp(int32(PY*L->PixelHeight),0,L->PixelHeight-1)*L->PixelWidth+FMath::Clamp(int32(PX*L->PixelWidth),0,L->PixelWidth-1)];if(Ink.A==0)continue;
                int32 K=(V+(Face/2)*Tile)*Dim+U+(Face%2)*Tile;FLinearColor A=FLinearColor::FromSRGBColor(Ink),B=FLinearColor::FromSRGBColor(Rendered[K]);float Alpha=A.A+B.A*(1-A.A);
                FLinearColor Out=(A*A.A+B*B.A*(1-A.A))/FMath::Max(.0001f,Alpha);Out.A=Alpha;Rendered[K]=Out.ToFColorSRGB();
            }
        }
    }
    Texture=UTexture2D::CreateTransient(Dim,Dim,PF_B8G8R8A8,NAME_None,TConstArrayView64<uint8>((const uint8*)Rendered.GetData(),Rendered.Num()*sizeof(FColor)));
    if(Texture){Texture->SRGB=true;Texture->AddressX=TA_Clamp;Texture->AddressY=TA_Clamp;Texture->NeverStream=true;Texture->UpdateResource();}
}
bool UBenchyLogo::SaveLayers(){
    IFileManager::Get().MakeDirectory(*LayerDir(),true);auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("version"),2);J->SetNumberField(TEXT("active"),ActiveIndex);TArray<TSharedPtr<FJsonValue>> Entries;
    for(int32 N=0;N<Layers.Num();++N){auto L=Layers[N];FString File=FString::Printf(TEXT("Layer-%d.png"),N);if(L->Original.IsEmpty()||!FFileHelper::SaveArrayToFile(L->Original,*(LayerDir()/File))){Notice=TEXT("Logo-Ebene konnte nicht gespeichert werden.");return false;}
        auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("file"),File);O->SetStringField(TEXT("name"),L->Name);O->SetBoolField(TEXT("enabled"),L->Enabled);O->SetNumberField(TEXT("side"),L->Side);O->SetNumberField(TEXT("x"),L->X);O->SetNumberField(TEXT("y"),L->Y);O->SetNumberField(TEXT("size"),L->Size);O->SetNumberField(TEXT("angle"),L->Angle);O->SetNumberField(TEXT("outline"),L->Outline);O->SetNumberField(TEXT("outline_width"),L->OutlineWidth);Entries.Add(MakeShared<FJsonValueObject>(O));
    }J->SetArrayField(TEXT("layers"),Entries);FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));bool Good=FFileHelper::SaveStringToFile(Text,*(LayerDir()/TEXT("LogoLayers.json")));Notice=Good?TEXT("Alle Logo-Ebenen gespeichert."):TEXT("Speichern fehlgeschlagen.");return Good;
}
bool UBenchyLogo::LoadLayers(){
    FString Text;TSharedPtr<FJsonObject> J;if(!FFileHelper::LoadFileToString(Text,*(LayerDir()/TEXT("LogoLayers.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||!J)return false;
    const TArray<TSharedPtr<FJsonValue>>* Entries=nullptr;if(!J->TryGetArrayField(TEXT("layers"),Entries)||Entries->Num()>8)return false;Layers.Empty();Layered=true;Side=5;
    for(int32 N=0;N<Entries->Num();++N){auto O=(*Entries)[N]->AsObject();if(!O)continue;auto L=NewObject<UBenchyLogo>(this);if(!L->Import(LayerDir()/FString::Printf(TEXT("Layer-%d.png"),N),false))continue;
        double V;auto Read=[&](const TCHAR* Key,float Default,float Low,float High){return O->TryGetNumberField(Key,V)&&FMath::IsFinite(V)?FMath::Clamp(float(V),Low,High):Default;};L->Side=Read(TEXT("side"),0,0,4);L->X=Read(TEXT("x"),.5,0,1);L->Y=Read(TEXT("y"),.17,0,1);L->Size=Read(TEXT("size"),.28,.08,.9);L->Angle=Read(TEXT("angle"),0,-180,180);L->Outline=Read(TEXT("outline"),1,0,3);L->OutlineWidth=Read(TEXT("outline_width"),3,0,12);O->TryGetStringField(TEXT("name"),L->Name);O->TryGetBoolField(TEXT("enabled"),L->Enabled);L->Rebuild();Layers.Add(L);
    }double Index=0;J->TryGetNumberField(TEXT("active"),Index);ActiveIndex=FMath::IsFinite(Index)?FMath::Clamp(int32(Index),0,FMath::Max(0,Layers.Num()-1)):0;BuildAtlas(AtlasSize);Notice.Reset();return true;
}
