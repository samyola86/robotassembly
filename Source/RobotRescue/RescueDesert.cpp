#include "RescueGame.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/Light.h"
#include "Engine/SpotLight.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/TextureCube.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

void ARescueWorld::CaptureStandardRoom(){
 TArray<UPrimitiveComponent*> Components;GetComponents(Components);
 for(auto* C:Components){auto* M=Cast<UStaticMeshComponent>(C);if(M&&(BodyMeshes.Contains(M)||Chains.Contains(M)||M==LeverStick||M==LeverKnob))continue;StandardRoomGeometry.Add(C);}
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->IsA<ALight>()||It->IsA<ASkyLight>()||It->IsA<AExponentialHeightFog>())StandardRoomActors.Add(*It);
}

void ARescueWorld::BuildDesertBay(){
 auto Model=[&](const TCHAR* Name,FVector P,FRotator R=FRotator(0,180,0)){
  auto* M=NewObject<UStaticMeshComponent>(this);M->SetupAttachment(RootComponent);M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/%s.%s"),Name,Name)));M->SetRelativeLocation(P);M->SetRelativeRotation(R);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetVisibility(false);M->RegisterComponent();DesertGeometry.Add(M);return M;
 };
 Model(TEXT("DesertRoom"),FVector::ZeroVector);
 BuildWallDoodles(true);
 for(int Side:{-1,1}){
  Model(TEXT("DesertShelf"),FVector(Side*350,30,0),FRotator(0,180-Side*12,0));
  Model(TEXT("DesertLamp"),FVector(Side*235,-35,565));
  auto* Warm=GetWorld()->SpawnActor<ASpotLight>(FVector(Side*235,-35,548),FRotator(-90,0,0));
  auto* W=Cast<USpotLightComponent>(Warm->GetLightComponent());W->SetMobility(EComponentMobility::Movable);W->SetIntensityUnits(ELightUnits::Lumens);W->SetIntensity(1300);W->SetAttenuationRadius(1000);W->SetInnerConeAngle(13);W->SetOuterConeAngle(29);W->SetSourceRadius(14);W->SetUseTemperature(true);W->SetTemperature(3200);W->SetCastShadows(true);W->SetCastRaytracedShadows(ECastRayTracedShadow::Enabled);Warm->SetActorHiddenInGame(true);DesertLights.Add(Warm);
 }
 Model(TEXT("HeroLeverBase"),FVector(-365,-80,0));
 auto* Vista=Box(TEXT("Plane"),FVector(0,255,330),FVector(1080,610,1),FLinearColor::White,FRotator(0,180,90));
 Vista->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/M_DesertVista.M_DesertVista")));Vista->SetCastShadow(false);Vista->SetVisibility(false);DesertGeometry.Add(Vista);
 auto* Day=GetWorld()->SpawnActor<ARectLight>(FVector(0,170,420),(FVector(0,-220,200)-FVector(0,170,420)).Rotation());
 auto* D=Cast<URectLightComponent>(Day->GetLightComponent());D->SetMobility(EComponentMobility::Movable);D->SetIntensityUnits(ELightUnits::Lumens);D->SetIntensity(1800);D->SetSourceWidth(520);D->SetSourceHeight(380);D->SetAttenuationRadius(1800);D->SetLightColor(FLinearColor(.78f,.88f,1));D->SetCastShadows(true);D->SetCastRaytracedShadows(ECastRayTracedShadow::Enabled);Day->SetActorHiddenInGame(true);DesertLights.Add(Day);
 auto* Front=GetWorld()->SpawnActor<ARectLight>(FVector(0,-600,460),(FVector(0,0,220)-FVector(0,-600,460)).Rotation());
 auto* F=Cast<URectLightComponent>(Front->GetLightComponent());F->SetMobility(EComponentMobility::Movable);F->SetIntensityUnits(ELightUnits::Lumens);F->SetIntensity(850);F->SetSourceWidth(700);F->SetSourceHeight(380);F->SetAttenuationRadius(1400);F->SetLightColor(FLinearColor(1,.85f,.63f));F->SetCastShadows(true);F->SetCastRaytracedShadows(ECastRayTracedShadow::Enabled);Front->SetActorHiddenInGame(true);DesertLights.Add(Front);
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SourceType=SLS_SpecifiedCubemap;Sky->GetLightComponent()->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Game/Art/T_StudioAmbient.T_StudioAmbient")));Sky->GetLightComponent()->SetIntensity(.8f);Sky->GetLightComponent()->SetLightColor(FLinearColor(.72f,.84f,1));Sky->SetActorHiddenInGame(true);DesertLights.Add(Sky);
}

void ARescueWorld::UpdateDesertBay(){
 if(DesertActive==CarMode)return;DesertActive=CarMode;
 for(auto* C:StandardRoomGeometry)if(C)C->SetVisibility(!CarMode);
 for(auto* A:StandardRoomActors)if(A)A->SetActorHiddenInGame(CarMode);
 for(auto* C:DesertGeometry)if(C)C->SetVisibility(CarMode);
 for(auto* A:DesertLights)if(A)A->SetActorHiddenInGame(!CarMode);
}
