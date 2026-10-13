#include "BenchyLogo.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageCore.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
namespace {FString LogoDir(){return FPaths::ProjectSavedDir()/TEXT("Customization");}}
bool UBenchyLogo::Import(const FString& Path,bool Persist){
    int64 Bytes=IFileManager::Get().FileSize(*Path);
    if(Bytes<8||Bytes>10*1024*1024){Notice=TEXT("Bitte eine PNG bis 10 MB wählen.");return false;}
    TArray<uint8> Data;if(!FFileHelper::LoadFileToArray(Data,*Path)){Notice=TEXT("PNG konnte nicht gelesen werden.");return false;}
    static const uint8 Signature[]={137,80,78,71,13,10,26,10};
    if(FMemory::Memcmp(Data.GetData(),Signature,8)!=0){Notice=TEXT("Die Datei ist keine gültige PNG.");return false;}
    auto& Module=FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));auto Decoder=Module.CreateImageWrapper(EImageFormat::PNG);
    if(!Decoder->SetCompressed(Data.GetData(),Data.Num())||Decoder->GetWidth()<1||Decoder->GetHeight()<1||Decoder->GetWidth()>4096||Decoder->GetHeight()>4096||Decoder->GetWidth()*Decoder->GetHeight()>8*1024*1024){Notice=TEXT("PNG zu groß. Maximal 4096 Pixel pro Seite und 8 Megapixel.");return false;}
    FImage Decoded,Image;if(!Module.DecompressImage(Data.GetData(),Data.Num(),Decoded)){Notice=TEXT("Die PNG ist beschädigt oder nicht unterstützt.");return false;}
    float Scale=FMath::Min(1.f,1024.f/FMath::Max(Decoded.SizeX,Decoded.SizeY));
    Decoded.ResizeTo(Image,FMath::Max(1,FMath::RoundToInt(Decoded.SizeX*Scale)),FMath::Max(1,FMath::RoundToInt(Decoded.SizeY*Scale)),ERawImageFormat::BGRA8,EGammaSpace::sRGB);
    PixelWidth=Image.SizeX+32;PixelHeight=Image.SizeY+32;Source.Init(FColor(0,0,0,0),PixelWidth*PixelHeight);
    auto Pixels=Image.AsBGRA8();for(int32 Row=0;Row<Image.SizeY;Row++)FMemory::Memcpy(Source.GetData()+(Row+16)*PixelWidth+16,Pixels.GetData()+Row*Image.SizeX,Image.SizeX*sizeof(FColor));
    Original=MoveTemp(Data);Name=FPaths::GetCleanFilename(Path);Enabled=true;Rebuild();Notice=TEXT("PNG geladen. Position und Kontur anpassen.");
    return !Persist||Save();
}
void UBenchyLogo::Rebuild(){
    if(Source.IsEmpty())return;int32 Radius=Outline==0?0:FMath::Clamp(OutlineWidth,0,12);
    TArray<uint8> Horizontal,Alpha;Horizontal.SetNumUninitialized(Source.Num());Alpha.SetNumUninitialized(Source.Num());
    for(int32 V=0;V<PixelHeight;V++)for(int32 U=0;U<PixelWidth;U++){uint8 A=0;for(int32 K=FMath::Max(0,U-Radius);K<=FMath::Min(PixelWidth-1,U+Radius);K++)A=FMath::Max(A,Source[V*PixelWidth+K].A);Horizontal[V*PixelWidth+U]=A;}
    for(int32 V=0;V<PixelHeight;V++)for(int32 U=0;U<PixelWidth;U++){uint8 A=0;for(int32 K=FMath::Max(0,V-Radius);K<=FMath::Min(PixelHeight-1,V+Radius);K++)A=FMath::Max(A,Horizontal[K*PixelWidth+U]);Alpha[V*PixelWidth+U]=A;}
    FLinearColor Edge=Outline==2?FLinearColor::Black:Outline==3?FLinearColor(1,.85f,.02f):FLinearColor::White;
    Rendered.SetNumUninitialized(Source.Num());
    for(int32 K=0;K<Source.Num();K++){float A=Source[K].A/255.f,E=Radius>0?Alpha[K]/255.f:0,Out=A+E*(1-A);FLinearColor C=FLinearColor::FromSRGBColor(Source[K]);C=Out>0?(C*A+Edge*E*(1-A))/Out:FLinearColor::Black;C.A=Out;Rendered[K]=C.ToFColorSRGB();}
    auto NewTexture=UTexture2D::CreateTransient(PixelWidth,PixelHeight,PF_B8G8R8A8,NAME_None,TConstArrayView64<uint8>((const uint8*)Rendered.GetData(),Rendered.Num()*sizeof(FColor)));
    if(NewTexture){NewTexture->SRGB=true;NewTexture->AddressX=TA_Clamp;NewTexture->AddressY=TA_Clamp;NewTexture->NeverStream=true;NewTexture->UpdateResource();Texture=NewTexture;}
}
void UBenchyLogo::Apply(UStaticMeshComponent* Mesh){
    if(!Mesh||!Mesh->GetStaticMesh())return;auto Bounds=Mesh->GetStaticMesh()->GetBoundingBox();
    if(Layered)BuildAtlas(Bounds.GetSize());
    for(int32 K=0;K<Mesh->GetNumMaterials();K++)if(auto M=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(K))){
        M->SetScalarParameterValue(TEXT("LogoEnabled"),Enabled&&Texture?1:0);
        if(Texture){M->SetTextureParameterValue(TEXT("LogoImage"),Texture);M->SetScalarParameterValue(TEXT("LogoAspect"),float(PixelWidth)/PixelHeight);}
        M->SetScalarParameterValue(TEXT("LogoSide"),Side);
        M->SetVectorParameterValue(TEXT("LogoMin"),FLinearColor(Bounds.Min.X,Bounds.Min.Y,Bounds.Min.Z));
        M->SetVectorParameterValue(TEXT("LogoMax"),FLinearColor(Bounds.Max.X,Bounds.Max.Y,Bounds.Max.Z));
        M->SetVectorParameterValue(TEXT("LogoPlace"),FLinearColor(X,1-Y,Size,FMath::DegreesToRadians(Angle)));
    }
}
bool UBenchyLogo::Save(){
    if(Layered)return SaveLayers();
    IFileManager::Get().MakeDirectory(*LogoDir(),true);
    if(!Original.IsEmpty()&&!FFileHelper::SaveArrayToFile(Original,*(LogoDir()/TEXT("Logo.png")))){Notice=TEXT("Logo konnte nicht gespeichert werden.");return false;}
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("enabled"),Enabled);J->SetStringField(TEXT("name"),Name);
    J->SetNumberField(TEXT("side"),Side);J->SetNumberField(TEXT("x"),X);J->SetNumberField(TEXT("y"),Y);J->SetNumberField(TEXT("size"),Size);J->SetNumberField(TEXT("angle"),Angle);J->SetNumberField(TEXT("outline"),Outline);J->SetNumberField(TEXT("outline_width"),OutlineWidth);
    FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(J,Writer);
    bool Good=FFileHelper::SaveStringToFile(Text,*(LogoDir()/TEXT("Logo.json")));Notice=Good?TEXT("Logo und Platzierung gespeichert."):TEXT("Speichern fehlgeschlagen.");return Good;
}
void UBenchyLogo::Load(){
    if(LoadLayers())return;
    FString Text;TSharedPtr<FJsonObject> J;if(!FFileHelper::LoadFileToString(Text,*(LogoDir()/TEXT("Logo.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||!J.IsValid())return;
    double V;auto Read=[&](const TCHAR* Key,float Default,float Low,float High){return J->TryGetNumberField(Key,V)&&FMath::IsFinite(V)?FMath::Clamp(float(V),Low,High):Default;};
    Side=Read(TEXT("side"),0,0,3);X=Read(TEXT("x"),.5,0,1);Y=Read(TEXT("y"),.25,0,1);Size=Read(TEXT("size"),.34,.08,.9);Angle=Read(TEXT("angle"),0,-180,180);Outline=Read(TEXT("outline"),1,0,3);OutlineWidth=Read(TEXT("outline_width"),3,0,12);
    bool Active=false;J->TryGetBoolField(TEXT("enabled"),Active);FString SavedName;J->TryGetStringField(TEXT("name"),SavedName);
    if(Import(LogoDir()/TEXT("Logo.png"),false)){Enabled=Active;Name=SavedName;Notice.Reset();}
}
void UBenchyLogo::Remove(){Enabled=false;Save();Notice=TEXT("Logo ausgeblendet. Mit PNG laden kannst du es ersetzen.");}
TArray<uint8> UBenchyLogo::NetworkPNG(){
    if(Layered)BuildAtlas(AtlasSize);
    TArray<uint8> Result;if(!Enabled||Rendered.IsEmpty())return Result;
    FImage SourceImage(PixelWidth,PixelHeight,ERawImageFormat::BGRA8,EGammaSpace::sRGB);FMemory::Memcpy(SourceImage.RawData.GetData(),Rendered.GetData(),Rendered.Num()*sizeof(FColor));
    auto& Module=FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    for(int32 Limit:{512,256,128,64}){FImage Small;float Scale=FMath::Min(1.f,float(Limit)/FMath::Max(PixelWidth,PixelHeight));SourceImage.ResizeTo(Small,FMath::Max(1,FMath::RoundToInt(PixelWidth*Scale)),FMath::Max(1,FMath::RoundToInt(PixelHeight*Scale)),ERawImageFormat::BGRA8,EGammaSpace::sRGB);TArray64<uint8> Compressed;
        if(Module.CompressImage(Compressed,EImageFormat::PNG,Small)&&Compressed.Num()<=65536){Result.Append(Compressed.GetData(),Compressed.Num());return Result;}}
    return Result;
}
bool UBenchyLogo::ImportNetwork(const TArray<uint8>& PNG){
    if(PNG.Num()<8||PNG.Num()>65536)return false;static const uint8 Sig[]={137,80,78,71,13,10,26,10};if(FMemory::Memcmp(PNG.GetData(),Sig,8)!=0)return false;
    auto& Module=FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));auto Decoder=Module.CreateImageWrapper(EImageFormat::PNG);
    if(!Decoder->SetCompressed(PNG.GetData(),PNG.Num())||Decoder->GetWidth()<1||Decoder->GetHeight()<1||Decoder->GetWidth()>512||Decoder->GetHeight()>512)return false;
    FImage Raw,Image;if(!Module.DecompressImage(PNG.GetData(),PNG.Num(),Raw))return false;Raw.CopyTo(Image,ERawImageFormat::BGRA8,EGammaSpace::sRGB);
    PixelWidth=Image.SizeX;PixelHeight=Image.SizeY;Source.SetNumUninitialized(PixelWidth*PixelHeight);FMemory::Memcpy(Source.GetData(),Image.RawData.GetData(),Source.Num()*sizeof(FColor));Outline=0;Enabled=true;Rebuild();return Texture!=nullptr;
}
