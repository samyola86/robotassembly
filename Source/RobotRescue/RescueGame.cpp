#include "RescueGame.h"
#include "ProceduralMeshComponent.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SpotLight.h"
#include "Components/SpotLightComponent.h"
#include "HAL/IConsoleManager.h"
#include "Components/RectLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/Canvas.h"
#include "Engine/TextureCube.h"
#include "Engine/Texture2D.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformFileManager.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundWave.h"

static const FLinearColor Steel(.28f,.35f,.39f),Dark(.025f,.044f,.06f),Silver(.62f,.69f,.71f),Gold(.95f,.38f,.055f),Cyan(.035f,.72f,1.f),Green(.15f,1.f,.57f),White(.88f,.94f,.97f);
static UMaterialInstanceDynamic* Material(UObject* Owner,FLinearColor Color,bool Glow){
 UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,Glow?TEXT("/Game/Art/M_Glow.M_Glow"):TEXT("/Game/Art/M_HeroWorld.M_HeroWorld"));
 if(!Base)Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 auto* M=UMaterialInstanceDynamic::Create(Base,Owner); if(M){M->SetVectorParameterValue(TEXT("Color"),Color);M->SetScalarParameterValue(TEXT("Roughness"),.45f);} return M;
}
static UStaticMesh* RescueMesh(const FString& Shape){
 FString Name=Shape==TEXT("Cube")?TEXT("MachinedPanel"):(Shape==TEXT("Cylinder")?TEXT("MachinedCylinder"):(Shape==TEXT("Link")?TEXT("ChainLink"):TEXT("")));
 if(!Name.IsEmpty()){auto* M=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Art/")+Name+TEXT(".")+Name));if(M)return M;}
 return LoadObject<UStaticMesh>(nullptr,*(TEXT("/Engine/BasicShapes/")+Shape+TEXT(".")+Shape));
}
ARobotPart::ARobotPart(){RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));}
UStaticMeshComponent* ARobotPart::Piece(const FString& Shape,FVector Pos,FVector Size,FLinearColor Color,FRotator Rot,bool Glow){
 auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(RescueMesh(Shape));C->SetRelativeLocation(Pos);C->SetRelativeScale3D(Size/100.f);C->SetRelativeRotation(Rot);C->SetMaterial(0,Material(this,Color,Glow));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(true);C->RegisterComponent();return C;
}
static UStaticMeshComponent* HeroComponent(AActor* Owner,USceneComponent* Root,const FString& Name,FVector Pos){
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Art/")+Name+TEXT(".")+Name));if(!Mesh)return nullptr;
 auto* C=NewObject<UStaticMeshComponent>(Owner);C->SetupAttachment(Root);C->SetStaticMesh(Mesh);C->SetRelativeLocation(Pos);C->SetRelativeRotation(FRotator(0,180,0));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();return C;
}
void ARobotPart::Make(int32 Kind){
 Id=Kind;
 const TCHAR* HeroNames[]={TEXT("HeroLeftArm"),TEXT("HeroRightArm"),TEXT("HeroLeftLeg"),TEXT("HeroRightLeg"),TEXT("HeroLargeGear"),TEXT("HeroSmallGear"),TEXT("HeroMotor"),TEXT("HeroBattery"),TEXT("HeroHead")};
 if(HeroComponent(this,RootComponent,HeroNames[Kind],FVector::ZeroVector))return;
 if(Kind<2){
  float S=Kind==0?-1:1;
  Piece("Sphere",FVector(0,0,30),FVector(38),Dark);Piece("Cube",FVector(S*5,0,5),FVector(41,45,62),Silver,FRotator(S*8,0,0));
  Piece("Cube",FVector(S*5,-24,14),FVector(24,5,26),Gold);Piece("Cylinder",FVector(S*8,0,-30),FVector(27,27,34),Dark,FRotator(0,0,90));
  Piece("Cube",FVector(S*9,0,-65),FVector(34,39,50),Steel);Piece("Cube",FVector(S*9,-21,-65),FVector(24,5,21),Silver);
  Piece("Cube",FVector(S*10,0,-105),FVector(29,34,25),Dark);
  for(int I=0;I<3;I++)Piece("Cube",FVector(S*10+(I-1)*10,-7,-125),FVector(7,16,24),Silver,FRotator((I-1)*8,0,0));
 }else if(Kind<4){
  Piece("Sphere",FVector(0,0,10),FVector(30),Dark);Piece("Cube",FVector(0,0,-24),FVector(42,44,60),Silver);
  Piece("Cylinder",FVector(0,-4,-59),FVector(30,30,38),Dark,FRotator(0,0,90));Piece("Cube",FVector(0,0,-96),FVector(45,46,54),Steel);
  Piece("Cube",FVector(0,-25,-94),FVector(28,6,32),Silver);Piece("Cube",FVector(0,-18,-133),FVector(53,78,25),Silver);Piece("Cube",FVector(0,-59,-135),FVector(48,5,9),Gold);
 }else if(Kind==4||Kind==5){
  float R=Kind==4?34:23;Piece("Cylinder",FVector::ZeroVector,FVector(R*1.55f,R*1.55f,13),Gold,FRotator(0,0,90));
  for(int I=0;I<12;I++){float A=I*PI/6;Piece("Cube",FVector(FMath::Sin(A)*R,0,FMath::Cos(A)*R),FVector(13,17,13),Gold,FRotator(-I*30,0,0));}
  Piece("Cylinder",FVector(0,-10,0),FVector(R*.55f,R*.55f,14),Dark,FRotator(0,0,90));Piece("Cylinder",FVector(0,-18,0),FVector(8,8,7),Silver,FRotator(0,0,90));
 }else if(Kind==6){
  Piece("Cylinder",FVector::ZeroVector,FVector(42,42,65),Silver,FRotator(90,0,0));Piece("Cylinder",FVector(-36,0,0),FVector(46,46,9),Dark,FRotator(90,0,0));Piece("Cylinder",FVector(43,0,0),FVector(14,14,27),Gold,FRotator(90,0,0));
  for(int I=-1;I<=1;I++)Piece("Cube",FVector(I*13,-23,0),FVector(5,5,22),Dark);
 }else if(Kind==7){
  Piece("Cube",FVector::ZeroVector,FVector(82,35,47),Dark);Piece("Cube",FVector(0,-20,0),FVector(66,5,29),Gold);
  Piece("Cube",FVector(-25,0,30),FVector(13,18,13),FLinearColor(.8f,.045f,.035f));Piece("Cube",FVector(25,0,30),FVector(13,18,13),Dark);
 }else{
  Piece("Cube",FVector::ZeroVector,FVector(106,65,74),Silver);Piece("Cube",FVector(0,-35,0),FVector(90,8,56),Dark);
  Piece("Cube",FVector(-24,-41,7),FVector(22,5,9),Cyan,FRotator::ZeroRotator,true);Piece("Cube",FVector(24,-41,7),FVector(22,5,9),Cyan,FRotator::ZeroRotator,true);
  Piece("Cube",FVector(0,-41,-18),FVector(22,5,4),Cyan,FRotator::ZeroRotator,true);
  Piece("Cube",FVector(-12,-41,-14),FVector(4,5,9),Cyan,FRotator::ZeroRotator,true);Piece("Cube",FVector(12,-41,-14),FVector(4,5,9),Cyan,FRotator::ZeroRotator,true);Piece("Cylinder",FVector(0,0,43),FVector(16,16,15),Dark);Piece("Cylinder",FVector(19,0,58),FVector(5,5,33),Gold);
  Piece("Sphere",FVector(19,0,76),FVector(9),Cyan);Piece("Cylinder",FVector(-58,0,0),FVector(35,35,13),Gold,FRotator(90,0,0));Piece("Cylinder",FVector(58,0,0),FVector(35,35,13),Gold,FRotator(90,0,0));
 }
}
ARescueWorld::ARescueWorld(){PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));}
UStaticMeshComponent* ARescueWorld::Box(FString Shape,FVector Pos,FVector Size,FLinearColor Color,FRotator Rot,bool Glow){
 auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(RescueMesh(Shape));C->SetRelativeLocation(Pos);C->SetRelativeScale3D(Size/100.f);C->SetRelativeRotation(Rot);C->SetMaterial(0,Material(this,Color,Glow));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();if(BuildingBody){BodyMeshes.Add(C);BodyRest.Add(Pos);}return C;
}
// Display racks share one transform with their parts, so the angled bays stay coherent.
static FVector RackPosition(FVector P){float Side=P.X>0?1.f:-1.f;return FVector(Side*425,60,300)+FRotator(0,-Side*20,0).RotateVector(P-FVector(Side*350,60,300));}
static void PaintedQuad(AActor* Owner,FVector A,FVector B,FVector C,FVector D,FLinearColor Color){
 auto* M=NewObject<UProceduralMeshComponent>(Owner);M->SetupAttachment(Owner->GetRootComponent());M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCastShadow(false);M->RegisterComponent();
 TArray<FVector> V={A,B,C,D},N={FVector(0,-1,0),FVector(0,-1,0),FVector(0,-1,0),FVector(0,-1,0)};TArray<int32> T={0,2,1,0,3,2};TArray<FVector2D> UV={FVector2D(0,0),FVector2D(1,0),FVector2D(1,1),FVector2D(0,1)};TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
 M->CreateMeshSection_LinearColor(0,V,T,N,UV,Colors,Tangents,false);M->SetMaterial(0,Material(Owner,Color,false));
}
void ARescueWorld::BuildRoom(){
 // Warm worn concrete slabs, laid as real floor geometry with narrow recessed seams.
 Box("Cube",FVector(0,0,-20),FVector(1800,1800,32),FLinearColor(.16f,.14f,.115f));
 for(int X=-7;X<=7;X++)for(int Y=-9;Y<=3;Y++){
  float V=((X*17+Y*13+1000)%7)*.008f;
  auto* Tile=Box("Cube",FVector(X*100,Y*100,-3),FVector(99,99,6),FLinearColor(.28f+V,.255f+V,.215f+V));
  auto* Mat=Cast<UMaterialInstanceDynamic>(Tile->GetMaterial(0));Mat->SetScalarParameterValue("Roughness",.56f);Mat->SetScalarParameterValue("Metallic",.08f);
 }
 // Continuous factory wall, with thin structural joints and painted service markings.
 Box("Cube",FVector(0,255,345),FVector(1600,35,800),FLinearColor(.18f,.19f,.18f));
 for(int X:{-650,-325,325,650})Box("Cube",FVector(X,235,340),FVector(3,4,720),FLinearColor(.08f,.085f,.075f));
 for(int Z:{180,410,660})Box("Cube",FVector(0,234,Z),FVector(1500,4,3),FLinearColor(.09f,.095f,.085f));
 for(int Side:{-1,1}){
  // Structural end posts with flush diagonal yellow factory paint, not floating blocks.
  float X=Side*565;
  Box("Cube",FVector(X,35,340),FVector(48,76,680),FLinearColor(.085f,.075f,.055f));
  Box("Cube",FVector(X,30,18),FVector(75,96,32),Steel);
  Box("Cube",FVector(X,30,650),FVector(68,96,30),Steel);
  for(int Z=45;Z<590;Z+=72)PaintedQuad(this,FVector(X-23,-3.5f,Z),FVector(X+23,-3.5f,Z+46),FVector(X+23,-3.5f,Z+74),FVector(X-23,-3.5f,Z+28),FLinearColor(.82f,.47f,.045f));
  for(int J=0;J<5;J++){float X0=Side*(110+J*34);PaintedQuad(this,FVector(X0-14,235,125),FVector(X0+10,235,125),FVector(X0+49,235,181),FVector(X0+25,235,181),FLinearColor(.31f,.26f,.10f));}
  auto* Cabinet=HeroComponent(this,RootComponent,TEXT("HeroCabinet"),FVector(Side*425,60,300));if(Cabinet){Cabinet->SetRelativeScale3D(FVector(1.4f,1.f,1.f));Cabinet->SetRelativeRotation(FRotator(0,180-Side*20,0));}
  FVector P=RackPosition(FVector(Side*350,-2,538));auto* L=GetWorld()->SpawnActor<ARectLight>(P,FRotator(-75,90-Side*20,0));auto* LC=Cast<URectLightComponent>(L->GetLightComponent());LC->SetMobility(EComponentMobility::Movable);LC->SetWorldRotation(FRotator(-75,90-Side*20,0));LC->SetIntensity(800);LC->SetAttenuationRadius(420);LC->SetSourceWidth(55);LC->SetSourceHeight(10);LC->SetCastShadows(true);LC->SetCastRaytracedShadows(ECastRayTracedShadow::Enabled);LC->SetLightColor(FLinearColor(.55f,.85f,1.f));
 }
 // Painted gear emblem on the wall: flat, desaturated industrial signage.
 for(int I=0;I<48;I++){float A=I*2*PI/48,B=(I+1)*2*PI/48;float R=(I%4<2?64:54),R1=((I+1)%4<2?64:54);PaintedQuad(this,FVector(FMath::Cos(A)*R,234,535+FMath::Sin(A)*R),FVector(FMath::Cos(B)*R1,234,535+FMath::Sin(B)*R1),FVector(FMath::Cos(B)*34,234,535+FMath::Sin(B)*34),FVector(FMath::Cos(A)*34,234,535+FMath::Sin(A)*34),FLinearColor(.29f,.29f,.245f));}
 for(int X:{-155,155})for(int Y:{-110,110}){
  Box("Cube",FVector(X,Y,.4f),FVector(65,7,.6f),FLinearColor(.82f,.48f,.06f));
  Box("Cube",FVector(X+(X>0?29:-29),Y+(Y>0?-22:22),.4f),FVector(7,50,.6f),FLinearColor(.82f,.48f,.06f));
 }
 auto* Torso=HeroComponent(this,RootComponent,TEXT("HeroTorso"),FVector(0,0,310));if(Torso){BodyMeshes.Add(Torso);BodyRest.Add(FVector(0,0,310));}
 BuildingBody=false;
 for(int S:{-1,1})for(int T:{0,1}){
  FVector Start(S*(T?52:93),10,T?213:380),End(S*(T?140:160),35,T?10:650);
  for(int I=0;I<24;I++){FVector P=FMath::Lerp(Start,End,I/23.f);auto* C=Box("Link",P,FVector(13,8,29),Steel,FRotator(S*(T?-29:23),I%2?70:0,0));Chains.Add(C);}
 }
 HeroComponent(this,RootComponent,TEXT("HeroLeverBase"),FVector(-365,-80,0));
 LeverStick=Box("Cylinder",FVector(-365,-80,125),FVector(8,8,100),Silver);LeverKnob=Box("Sphere",FVector(-365,-80,175),FVector(34),FLinearColor(.55f,.018f,.012f));
 Box("Sphere",FVector(-365,-80,75),FVector(24),Dark);
 Box("Cylinder",FVector(-365,-80,76),FVector(35,35,6),Steel);
 for(int Side:{-1,1}){
  Box("Cylinder",FVector(Side*245,-70,612),FVector(92,92,24),Dark);
  Box("Cylinder",FVector(Side*245,-70,599),FVector(104,104,5),Steel);
  Box("Cylinder",FVector(Side*245,-70,596),FVector(94,94,3),FLinearColor(1.f,.72f,.35f),FRotator::ZeroRotator,true);
  Box("Cylinder",FVector(Side*245,-70,710),FVector(5,5,190),Steel);
  // Lamp beam starts inside its luminous aperture and aims directly at the floor below.
  auto* Warm=GetWorld()->SpawnActor<ASpotLight>(FVector(Side*245,-70,570),FRotator(-90,0,0));auto* WC=Cast<USpotLightComponent>(Warm->GetLightComponent());WC->SetMobility(EComponentMobility::Movable);WC->SetWorldRotation(FRotator(-90,0,0));WC->SetIntensityUnits(ELightUnits::Lumens);WC->SetIntensity(3500);WC->SetAttenuationRadius(1100);WC->SetInnerConeAngle(12);WC->SetOuterConeAngle(23);WC->SetSourceRadius(15);WC->SetSoftSourceRadius(8);WC->SetCastShadows(true);WC->SetCastRaytracedShadows(ECastRayTracedShadow::Enabled);WC->SetSamplesPerPixel(4);WC->SetUseTemperature(true);WC->SetTemperature(3400);

  Box("Cylinder",FVector(Side*480,160,330),FVector(17,17,650),Steel);
  for(int J=0;J<10;J++)Box("Cube",FVector(Side*470,180,120+J*35),FVector(60,12,8),Dark);
  Box("Cube",FVector(Side*70,170,23),FVector(38,12,5),Cyan,FRotator::ZeroRotator,true);
  auto* Blue=GetWorld()->SpawnActor<APointLight>(FVector(Side*90,80,60),FRotator::ZeroRotator);Blue->PointLightComponent->SetIntensity(120);Blue->PointLightComponent->SetAttenuationRadius(230);Blue->PointLightComponent->SetSourceRadius(30);Blue->PointLightComponent->SetLightColor(Cyan);
 }
 // A pair of visible warm work lights provides soft frontal illumination.
 // Unlike the old unmotivated fill, the lenses, housings, direction and ray shadows agree.
 for(int Side:{-1,1}){
  FVector P(Side*550,-240,520);
  Box("Cube",P,FVector(36,25,48),Dark);
  Box("Cube",P+FVector(0,14,0),FVector(28,3,38),FLinearColor(1.f,.81f,.52f),FRotator::ZeroRotator,true);
  auto* Work=GetWorld()->SpawnActor<ARectLight>(P+FVector(0,18,0),(FVector(0,0,260)-P).Rotation());auto* C=Cast<URectLightComponent>(Work->GetLightComponent());C->SetMobility(EComponentMobility::Movable);C->SetWorldRotation((FVector(0,0,260)-P).Rotation());C->SetIntensityUnits(ELightUnits::Lumens);C->SetIntensity(450);C->SetAttenuationRadius(1400);C->SetSourceWidth(28);C->SetSourceHeight(38);C->SetCastShadows(true);C->SetCastRaytracedShadows(ECastRayTracedShadow::Enabled);C->SetSamplesPerPixel(4);C->SetUseTemperature(true);C->SetTemperature(4300);
 }

 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();auto* Mist=Fog->FindComponentByClass<UExponentialHeightFogComponent>();if(Mist){Mist->SetFogDensity(.013f);Mist->SetFogHeightFalloff(.05f);Mist->SetFogMaxOpacity(.15f);Mist->SetStartDistance(500);Mist->SetFogInscatteringColor(FLinearColor(.12f,.105f,.08f));}
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SourceType=SLS_SpecifiedCubemap;Sky->GetLightComponent()->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Game/Art/T_StudioAmbient.T_StudioAmbient")));Sky->GetLightComponent()->SetIntensity(.5f);Sky->GetLightComponent()->SetLightColor(FLinearColor(1.f,.85f,.65f));
}
void ARescueWorld::BeginPlay(){
 Super::BeginPlay();SaveFile=FPaths::ProjectSavedDir()/TEXT("RobotRescueProgress.ini");
 FString Labels[]={"LEFT ARM","RIGHT ARM","LEFT LEG","RIGHT LEG","LARGE GEAR","SMALL GEAR","MOTOR","BATTERY","HEAD"};for(int I=0;I<9;I++)Names[I]=Labels[I];
 BuildRoom();
 BuildDisco();
 // Small foil ribbons live in the workshop and pick up its actual lighting.
 const FLinearColor FoilColors[]={FLinearColor(.95f,.59f,.16f),FLinearColor(.82f,.86f,.9f),FLinearColor(.07f,.49f,.53f),FLinearColor(.68f,.25f,.10f)};
 for(int I=0;I<112;I++){
  auto* Ribbon=Box("Cube",FVector::ZeroVector,FVector(4.1f+(I%3)*.7f,10.f+(I%5)*1.1f,.32f),FoilColors[I%4]);
  Ribbon->SetCastShadow(false);Ribbon->SetVisibility(false);FinaleFoil.Add(Ribbon);
  if(auto* Mat=Cast<UMaterialInstanceDynamic>(Ribbon->GetMaterial(0))){Mat->SetScalarParameterValue(TEXT("Metallic"),.50f);Mat->SetScalarParameterValue(TEXT("Roughness"),.37f);}
 }
 for(const TCHAR* Name:{TEXT("r.RayTracing"),TEXT("r.RayTracing.Shadows"),TEXT("r.Lumen.HardwareRayTracing")}){auto* V=IConsoleManager::Get().FindConsoleVariable(Name);UE_LOG(LogTemp,Display,TEXT("RR_RENDER %s=%d"),Name,V?V->GetInt():-1);}

 HeroComponent(this,RootComponent,TEXT("HeroCableRoll"),RackPosition(FVector(-350,0,123)));
 FVector Homes[]={FVector(350,0,395),FVector(-350,0,510),FVector(-390,0,400),FVector(-320,0,400),FVector(369,-10,250),FVector(327,-12,230),FVector(350,-10,123),FVector(-350,0,241),FVector(350,-15,480)};
 FVector Sockets[]={FVector(-103,0,349),FVector(103,0,349),FVector(-43,0,188),FVector(43,0,188),FVector(-28,-58,331),FVector(25,-58,298),FVector(0,-58,271),FVector(0,-57,235),FVector(0,0,463)};
 for(int I=0;I<9;I++){auto* P=GetWorld()->SpawnActor<ARobotPart>();P->Home=RackPosition(Homes[I]);P->Socket=Sockets[I];P->SetActorLocation(P->Home);P->Make(I);Parts.Add(P);}
 BuildRobotStyle();
 BuildCarLevel();
 Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->SetActorLocation(FVector(0,-1350,470));Camera->SetActorRotation((FVector(0,0,315)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->FieldOfView=54;
 Camera->GetCameraComponent()->PostProcessSettings.bOverride_AutoExposureBias=true;Camera->GetCameraComponent()->PostProcessSettings.AutoExposureBias=-1.0f;
 auto* PC=GetWorld()->GetFirstPlayerController();if(PC){PC->SetViewTarget(Camera);PC->bShowMouseCursor=true;}
 Voice=NewObject<UAudioComponent>(this);Voice->RegisterComponent();Tone=NewObject<USoundWaveProcedural>(this);Tone->SetSampleRate(22050);Tone->NumChannels=1;Tone->Duration=INDEFINITELY_LOOPING_DURATION;ToneAudio=NewObject<UAudioComponent>(this);ToneAudio->RegisterComponent();ToneAudio->SetSound(Tone);ToneAudio->Play();
 Music=NewObject<UAudioComponent>(this);Music->RegisterComponent();Cheer=NewObject<UAudioComponent>(this);Cheer->RegisterComponent();
 Effects=NewObject<UAudioComponent>(this);Effects->RegisterComponent();
 Plasma=NewObject<UProceduralMeshComponent>(this);Plasma->SetupAttachment(RootComponent);Plasma->SetCollisionEnabled(ECollisionEnabled::NoCollision);Plasma->SetCastShadow(false);Plasma->RegisterComponent();
 PlasmaMaterial=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/M_PlasmaFilament.M_PlasmaFilament")),this);Plasma->SetMaterial(0,PlasmaMaterial);
 PlasmaLight=NewObject<UPointLightComponent>(this);PlasmaLight->SetupAttachment(RootComponent);PlasmaLight->SetRelativeLocation(FVector(0,-145,340));PlasmaLight->SetAttenuationRadius(260);PlasmaLight->SetSourceRadius(55);PlasmaLight->SetCastShadows(false);PlasmaLight->SetLightColor(FLinearColor(.32f,.40f,1.f));PlasmaLight->SetIntensity(0);PlasmaLight->RegisterComponent();
 Load();Stage=0;MenuPage=0;CarMode=false;Refresh();
 if(FParse::Param(FCommandLine::Get(),TEXT("RRTest"))){RunTests();}
 int Preview=-1;if(FParse::Value(FCommandLine::Get(),TEXT("RRPreview="),Preview)){
  Stage=Preview;for(int I=0;I<9;I++)Parts[I]->Installed=(I<4&&Stage>1)||(I<6&&Stage>2)||(I==6&&Stage>3)||(I==7&&Stage>4)||(I==8&&Stage>6);Red=Black=Stage>5;Refresh();
  if(Stage==10)FirstClear=true;
  if(Stage>=9)for(auto* Chain:Chains)Chain->SetVisibility(false);
  int JobPreview=0;
  if(FParse::Value(FCommandLine::Get(),TEXT("RRJobPreview="),JobPreview)){
   if(Stage==10&&JobPreview==4){
    OpenCarLevel();int CarPreview=0;
    if(FParse::Value(FCommandLine::Get(),TEXT("RRCarPreview="),CarPreview)){
     if(CarPreview>=1){CarInstalled=15;CarEnginePlaced=true;CarEnginePuzzle=CarPreview==1;}
     if(CarPreview>=2){CarCylinders=3;CarInstalled=31;CarEnginePuzzle=false;CarWiring=CarPreview==2;}
     if(CarPreview>=3){CarWired=true;CarHead=true;CarReleased=true;CarComplete=true;CarWiring=false;CarTestTime=StageTime;}
     UpdateCarLevel();
    }
   }
   else if(Stage==10&&JobPreview==3){JobMedals=3;Collection=true;RobotStyles[2].Accessory=1;SelectRobot(2);}
   else OpenWorkshopJob(JobPreview);
  }
 }
 int MenuPreview=-1;if(FParse::Value(FCommandLine::Get(),TEXT("RRMenuPreview="),MenuPreview)){ReturnToMenu();MenuPage=MenuPreview;FirstClear=MenuPreview==1;MenuLoadTime=Clock+60;PendingLevel=1;}
}
bool ARescueWorld::Available(int32 I)const{return Parts.IsValidIndex(I)&&!Parts[I]->Installed&&((Stage==1&&I<4)||(Stage==2&&(I==4||I==5))||(Stage==3&&I==6)||(Stage==4&&I==7)||(Stage==6&&I==8));}
bool ARescueWorld::Ready()const{if(!Red||!Black)return false;for(auto* P:Parts)if(!P->Installed)return false;return true;}
void ARescueWorld::Refresh(){
 for(int I=0;I<Parts.Num();I++){auto* P=Parts[I];P->SetActorHiddenInGame(Stage==0?false:(!P->Installed&&Stage>7));P->SetActorScale3D(FVector(P->Installed?1.f:((I<2)?.75f:(I<4?.64f:(I==8?.95f:.85f)))));if(!Dragging||Selected!=I)P->SetActorLocation(P->Installed?P->Socket:P->Home);P->SetActorRotation(P->Installed?FRotator::ZeroRotator:FRotator(I<2?(I==0?-40:40):0,P->Home.X>0?-20:20,0));}
 const TCHAR* Tasks[]={TEXT("Your friend needs an engineer."),TEXT("Rebuild your friend - match four limbs."),TEXT("Match each gear to its size."),TEXT("Fit the motor to the shaft connector."),TEXT("Match + and - on the battery."),TEXT("Connect positive and negative."),TEXT("One last piece - attach the head."),TEXT("Pull the lever to free your friend!"),TEXT("Stand clear. Your friend is landing safely."),TEXT("A new friendship starts with a dance."),TEXT("Mission 01 - The Awakening complete.")};
 Task=Tasks[FMath::Clamp(Stage,0,10)];
 StageTime=0;
 DanceQueue.Empty();DanceMove=-1;DanceMoveUntil=0;ManualFoilTime=-1;
 ApplyRobotStyle();
 UpdateMusic();
 if(LeverKnob)LeverKnob->SetMaterial(0,Material(this,FLinearColor(.55f,.018f,.012f),false));
}
void ARescueWorld::PlayEffect(const FString& Name){
 if(!Effects)return;Effects->SetSound(LoadObject<USoundWave>(nullptr,*(TEXT("/Game/Audio/")+Name+TEXT(".")+Name)));Effects->SetVolumeMultiplier(Quiet?0.f:.85f);Effects->Play();
}
void ARescueWorld::UpdateReleaseAudio(){
 if(Stage==8&&!YayStarted&&StageTime>=AliveDuration){YayStarted=true;Cheer->SetSound(LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/Yay.Yay")));Cheer->Play();}
}
void ARescueWorld::UpdateMusic(){
 if(!Music)return;
 int Wanted=(Stage==9||(CarMode&&CarReleased&&CarTestTime>=0&&StageTime-CarTestTime<10.f))?1:(Stage==8?2:0);
 if(Wanted!=MusicStage){Music->Stop();MusicStage=Wanted;if(Wanted!=2){const TCHAR* Path=Wanted==1?TEXT("/Game/Audio/PartyDance.PartyDance"):TEXT("/Game/Audio/ScienceMusic.ScienceMusic");Music->SetSound(LoadObject<USoundWave>(nullptr,Path));Music->Play();UE_LOG(LogTemp,Display,TEXT("RR_AUDIO %s"),Wanted==1?TEXT("PartyDance"):TEXT("ScienceMusic"));}}
 Music->SetPaused(Paused);Music->SetVolumeMultiplier(Quiet?0.f:(Voice&&Voice->IsPlaying()?.12f:(Wanted==1?.65f:.28f)));
}
void ARescueWorld::Say(const FString& S,const FString& Clip){Line=S;VoiceTime=Clock; if(Quiet||Clip.IsEmpty())return;auto* Sound=LoadObject<USoundWave>(nullptr,*(TEXT("/Game/Audio/")+Clip+TEXT(".")+Clip));if(Sound){Voice->SetSound(Sound);Voice->Play();}}
void ARescueWorld::Beep(float F){if(Quiet)return;if(F<20)F=700;TArray<int16> PCM;int N=3300;PCM.SetNum(N);for(int I=0;I<N;I++)PCM[I]=(int16)(FMath::Sin(2*PI*F*I/22050)*1600*(1.f-I/(float)N));Tone->QueueAudio((uint8*)PCM.GetData(),N*2);}
void ARescueWorld::Advance(){Stage++;Selected=-1;WireStart=-1;Failures=0;Refresh();Save();}
bool ARescueWorld::TryPlace(int32 I,int32 Target){
 if(!Available(I))return false;
 if(I!=Target||(I==7&&BatteryBackward)){
  Failures++;Say(I==7&&I==Target?TEXT("Match the plus and minus symbols. Turn the battery around."):TEXT("Almost! Look for the matching shape and symbol."),I==7?TEXT("BatteryHint"):TEXT("Mismatch"));Beep(220);return false;
 }
 Parts[I]->Installed=true;Parts[I]->SetActorScale3D(FVector(1.f));Parts[I]->SetActorLocation(Parts[I]->Socket);Parts[I]->SetActorRotation(FRotator::ZeroRotator);Dragging=false;Selected=-1;
 if(ReactionPart>=0)Parts[ReactionPart]->SetActorRotation(FRotator::ZeroRotator);
 Reaction=Clock;ReactionPart=I<4?I:-1;
 Beep(830);Save();
 if(I<4)PlayEffect(TEXT("LimbMechanism"));
 if(I==4||I==5)PlayEffect(TEXT("GearAttach"));
 if(I==8){PlayEffect(TEXT("HeadAttach"));AwakenTime=0;}
 if(I<2)Say(TEXT("High five, engineer! Wait... I only have three fingers."),TEXT("Arm"));
 else if(I<4&&Parts[2]->Installed&&Parts[3]->Installed)Say(TEXT("Stomp test! Whoa, wobbly legs!"),TEXT("Limb"));
 else if(I<4)Say(TEXT("One foot down. One to go!"),TEXT("Limb"));
 if(Stage==1){bool All=true;for(int J=0;J<4;J++)All&=Parts[J]->Installed;if(All){Advance();Say(TEXT("All limbs fitted! Give my gears a turn next."),TEXT("Chest"));}}
 else if(Stage==2&&Parts[4]->Installed&&Parts[5]->Installed){Advance();Say(TEXT("Excellent! The gears transfer movement!"),TEXT("Gears"));}
 else if(Stage==3){Advance();Say(TEXT("Motor fitted. Now match the battery symbols."),TEXT("Motor"));}
 else if(Stage==4){Advance();Say(TEXT("In this robot, red connects positive. Black connects negative."),TEXT("Wiring"));}
 else if(Stage==6){Advance();Say(Nickname.IsEmpty()?TEXT("System online! You fixed me! You're an amazing engineer!"):FString::Printf(TEXT("System online! Hey, %s! You fixed me! You're an amazing engineer!"),*Nickname),TEXT("Online"));}
 return true;
}
bool ARescueWorld::Connect(int32 C,int32 A,int32 B){
 if(Stage!=5)return false;
 if((C==0&&A==0&&B==2)||(C==1&&A==1&&B==3)){
  if(C==0)Red=true;else Black=true;WireStart=-1;Beep(1000);Save();
  Say(C==0?TEXT("Positive connected. Now look for negative."):TEXT("Negative connected. You're building a working system."));
  if(Red&&Black){PlayEffect(TEXT("MotorEffect"));Advance();PowerCountdown=0;PowerCount=3;Say(TEXT("Systems waking up. Drumroll, please!"));Beep(560);}return true;
 }
 Failures++;WireStart=-1;Beep(200);Say(C==0?TEXT("Oops! Red goes to positive. Try again!"):TEXT("Black goes to negative. Look for the minus sign."),C==0?TEXT("RedHint"):TEXT("BlackHint"));return false;
}
void ARescueWorld::RecordPen(FVector2D P){if((Stage==5||CarWiring)&&!Paused&&PenDown&&P.X>=735&&P.X<=1345&&P.Y>=310&&P.Y<=600&&(PenPoints.IsEmpty()||FVector2D::Distance(PenPoints.Last(),P)>4))PenPoints.Add(P);}
void ARescueWorld::FinishTrack(int32 End){
 if(End<2||End>5||WireStart<0||WireStart>1)return;int C=End-2;ActiveTrack=C;PreviousTracks[C]=Tracks[C];PreviousStart[C]=TrackStart[C];PreviousEnd[C]=TrackEnd[C];
 if(PenPoints.Num()<2){PenPoints={Terminal(WireStart),(Terminal(WireStart)+Terminal(End))*.5f,Terminal(End)};}else PenPoints.Add(Terminal(End));
 Tracks[C]=PenPoints;TrackStart[C]=WireStart;TrackEnd[C]=End;PenDown=false;WireStart=-1;PenPoints.Empty();CircuitPassed=false;TestStarted=-100;MonitorMessage=TEXT("Track drawn. Test your circuit when all four are ready.");Beep(650);
}
void ARescueWorld::UndoTrack(){int C=ActiveTrack;Swap(Tracks[C],PreviousTracks[C]);Swap(TrackStart[C],PreviousStart[C]);Swap(TrackEnd[C],PreviousEnd[C]);CircuitPassed=false;TestStarted=-100;}
static float PointSegment(FVector2D P,FVector2D A,FVector2D B){auto D=B-A;float T=FMath::Clamp(FVector2D::DotProduct(P-A,D)/FMath::Max(D.SizeSquared(),.001f),0.f,1.f);return FVector2D::Distance(P,A+D*T);}
static bool SegmentsTouch(FVector2D A,FVector2D B,FVector2D C,FVector2D D,FVector2D& At){
 auto Cross=[](FVector2D U,FVector2D V){return U.X*V.Y-U.Y*V.X;};auto R=B-A,S=D-C;double Den=Cross(R,S);
 if(FMath::Abs(Den)>.0001){double T=Cross(C-A,S)/Den,U=Cross(C-A,R)/Den;if(T>=0&&T<=1&&U>=0&&U<=1){At=A+R*T;return true;}}
 for(auto P:{A,B})if(PointSegment(P,C,D)<=12){At=P;return true;}for(auto P:{C,D})if(PointSegment(P,A,B)<=12){At=P;return true;}return false;
}
bool ARescueWorld::TrackCollision(FVector2D& At) const {
 auto OutsidePad=[&](const TArray<FVector2D>& P,int Source){TArray<FVector2D> Out;for(int I=1;I<P.Num();I++){int Steps=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(P[I-1],P[I])/6));for(int K=0;K<=Steps;K++){auto Q=FMath::Lerp(P[I-1],P[I],K/(float)Steps);if(FVector2D::Distance(Q,Terminal(Source))>110)Out.Add(Q);}}return Out;};
 for(int A=0;A<4;A++)for(int B=A+1;B<4;B++){
  auto PA=A/2==B/2?OutsidePad(Tracks[A],A/2):Tracks[A];auto PB=A/2==B/2?OutsidePad(Tracks[B],B/2):Tracks[B];
  for(int I=1;I<PA.Num();I++)for(int J=1;J<PB.Num();J++)if(SegmentsTouch(PA[I-1],PA[I],PB[J-1],PB[J],At))return true;
 }return false;
}
void ARescueWorld::TestCircuit(){
 CircuitPassed=true;for(int C=0;C<4;C++)CircuitPassed&=Tracks[C].Num()>=2&&TrackStart[C]==(C<2?0:1)&&TrackEnd[C]==C+2;
 TracksTouch=TrackCollision(TouchPoint);
 CircuitBlocked=!TracksTouch&&TrackHitsObstacle(TouchPoint);
 CircuitPassed&=!TracksTouch&&!CircuitBlocked;
 TestStarted=StageTime;
 if(CircuitPassed)MonitorMessage=TEXT("Testing all four tracks... watch the power flow.");
 else if(CircuitBlocked)MonitorMessage=TEXT("A wire touches the striped guard. Try routing around it.");
 else if(TracksTouch)MonitorMessage=TEXT("Tracks touch at the marked ring. Erase one and leave a gap!");
 else MonitorMessage=TEXT("Red + feeds both A pads. Black - feeds both B pads.");
 Beep(CircuitPassed?900:250);
}
void ARescueWorld::SendCircuit(){
 if(!CircuitPassed||StageTime-TestStarted<2.4f){MonitorMessage=TEXT("Test both connections before sending power.");return;}
 if(CarWiring){CarWired=true;CarWiring=false;WireStart=-1;PlayEffect(TEXT("MotorEffect"));Say(TEXT("Engine power is connected! Now fit my headlight face."));Save();return;}
 Connect(0,0,2);Connect(1,1,3);
}
void ARescueWorld::Pick(int32 I){if(!Available(I))return;Selected=I;Dragging=false;Beep(450);Say(Names[I]+TEXT(" selected. Match its outlined socket."));}
static constexpr float DiscoBallRadius=46.f;
static const FLinearColor DiscoPalette[]={FLinearColor(.10f,.66f,1.f),FLinearColor(.88f,.15f,.52f),FLinearColor(.45f,.24f,1.f),FLinearColor(1.f,.67f,.23f),FLinearColor(.42f,.89f,1.f)};

void ARescueWorld::BuildDisco(){
 DiscoRig=NewObject<USceneComponent>(this);
 DiscoRig->SetupAttachment(RootComponent);
 DiscoRig->RegisterComponent();
 DiscoCable=Box("Cylinder",FVector::ZeroVector,FVector(3,3,100),Steel);
 DiscoCable->SetVisibility(false);

 // Separate flat mirror tiles catch distinct reflections as the ball spins.
 constexpr int Rings=14, Columns=28;
 TArray<FVector> Vertices, Normals;
 TArray<int32> Indices;
 TArray<FVector2D> UVs;
 TArray<FLinearColor> Colors;
 TArray<FProcMeshTangent> Tangents;
 auto OnSphere=[](float Polar,float Azimuth){
  return DiscoBallRadius*FVector(FMath::Sin(Polar)*FMath::Cos(Azimuth),FMath::Sin(Polar)*FMath::Sin(Azimuth),FMath::Cos(Polar));
 };
 for(int Ring=0;Ring<Rings;Ring++){
  for(int Column=0;Column<Columns;Column++){
   const float P0=PI*Ring/Rings,P1=PI*(Ring+1)/Rings;
   const float A0=2*PI*Column/Columns,A1=2*PI*(Column+1)/Columns;
   const FVector Corners[]={OnSphere(P0,A0),OnSphere(P1,A0),OnSphere(P1,A1),OnSphere(P0,A1)};
   const FVector Center=(Corners[0]+Corners[1]+Corners[2]+Corners[3])*.25f;
   const int Base=Vertices.Num();
   for(const FVector& Corner:Corners){
    Vertices.Add(FMath::Lerp(Center,Corner,.91f));
    Normals.Add(Center.GetSafeNormal());
   }
   UVs.Append({{0,0},{0,1},{1,1},{1,0}});
   Indices.Append({Base,Base+2,Base+1,Base,Base+3,Base+2});
  }
 }
 auto* Core=Box("Sphere",FVector::ZeroVector,FVector(89),FLinearColor(.09f,.10f,.13f));
 Core->AttachToComponent(DiscoRig,FAttachmentTransformRules::KeepRelativeTransform);
 Core->SetCastShadow(false);
 DiscoTiles=NewObject<UProceduralMeshComponent>(this);
 DiscoTiles->SetupAttachment(DiscoRig);
 DiscoTiles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 DiscoTiles->SetCastShadow(false);
 DiscoTiles->RegisterComponent();
 DiscoTiles->CreateMeshSection_LinearColor(0,Vertices,Indices,Normals,UVs,Colors,Tangents,false);
 if(auto* Mirror=Material(this,FLinearColor(.83f,.87f,.94f),false)){
  Mirror->SetScalarParameterValue(TEXT("Metallic"),.96f);
  Mirror->SetScalarParameterValue(TEXT("Roughness"),.20f);
  DiscoTiles->SetMaterial(0,Mirror);
 }
 DiscoRig->SetVisibility(false,true);

 for(int I=0;I<20;I++){
  auto* Spot=NewObject<USpotLightComponent>(this);
  Spot->SetupAttachment(RootComponent);
  Spot->SetIntensityUnits(ELightUnits::Candelas);
  Spot->SetIntensity(3900.f+(I%4)*450.f);
  Spot->SetAttenuationRadius(1200.f);
  Spot->SetInnerConeAngle(.6f);
  Spot->SetOuterConeAngle(1.6f+(I%3)*.35f);
  Spot->SetLightColor(DiscoPalette[I%5]);
  Spot->SetCastShadows(false);
  Spot->SetVisibility(false);
  Spot->RegisterComponent();
  DiscoSpots.Add(Spot);
 }
 for(int Side:{-1,1}){
  auto* Wash=NewObject<UPointLightComponent>(this);
  Wash->SetupAttachment(RootComponent);
  Wash->SetRelativeLocation(FVector(Side*285,-115,500));
  Wash->SetIntensityUnits(ELightUnits::Lumens);
  Wash->SetIntensity(1850);
  Wash->SetAttenuationRadius(880);
  Wash->SetLightColor(Side<0?DiscoPalette[0]:DiscoPalette[1]);
  Wash->SetCastShadows(false);
  Wash->SetVisibility(false);
  Wash->RegisterComponent();
  DiscoWash.Add(Wash);
 }
}

void ARescueWorld::UpdateDisco(){
 const bool Dancing=Stage==9;
 if(DiscoRig->IsVisible()!=Dancing){
  DiscoRig->SetVisibility(Dancing,true);
  DiscoCable->SetVisibility(Dancing);
 }
 const bool MovingLights=Dancing&&!Reduced&&StageTime>=.35f;
 for(auto* Spot:DiscoSpots){if(Spot->IsVisible()!=MovingLights)Spot->SetVisibility(MovingLights);}
 for(auto* Wash:DiscoWash){if(Wash->IsVisible()!=Dancing)Wash->SetVisibility(Dancing);}
 if(!Dancing)return;

 const float Drop=Reduced?1.f:FMath::SmoothStep(0.f,1.f,FMath::Clamp(StageTime/1.5f,0.f,1.f));
 const FVector Ball(0,25,FMath::Lerp(805.f,633.f,Drop));
 DiscoRig->SetRelativeLocation(Ball);
 DiscoTiles->SetRelativeRotation(FRotator(0,Reduced?0.f:StageTime*49.f,0));
 const float CableLength=FMath::Max(0.f,865.f-Ball.Z-44.f);
 DiscoCable->SetRelativeLocation(Ball+FVector(0,0,44.f+CableLength*.5f));
 DiscoCable->SetRelativeScale3D(FVector(.028f,.028f,CableLength/100.f));
 if(!MovingLights)return;
 if(LastBeat!=DanceMove){
  LastBeat=DanceMove;
  for(int I=0;I<DiscoSpots.Num();I++)DiscoSpots[I]->SetLightColor(DiscoPalette[(I+DanceMove+1)%5]);
 }

 // Aim the reflected spots at the floor and the back of the workshop.
 for(int I=0;I<DiscoSpots.Num();I++){
  const float MoveOffset=DanceMove>=0?FMath::Sin((StageTime-DanceMoveStart)*PI/1.75f)*(.22f+.16f*DanceMove):0.f;
  const float Phase=StageTime*.88f+I*2.39996f+MoveOffset;
  const float X=FMath::Cos(Phase)*(325.f+(I%4)*66.f);
  const FVector Target=I<12
   ? FVector(X,232,290.f+FMath::Sin(Phase)*200.f+(I%3)*38.f)
   : FVector(X,-188.f+FMath::Sin(Phase)*162.f,0);
  DiscoSpots[I]->SetRelativeLocation(Ball+(Target-Ball).GetSafeNormal()*DiscoBallRadius);
  DiscoSpots[I]->SetRelativeRotation((Target-Ball).Rotation());
 }
}

void ARescueWorld::UpdateFinaleFoil(){
 const bool Celebrating=Stage==9&&!Reduced&&(ManualFoilTime>=0||StageTime>=10.5f);
 for(int I=0;I<FinaleFoil.Num();I++){
  auto* Ribbon=FinaleFoil[I];FRandomStream R(1907+I*97);
  const float Side=I%2?1.f:-1.f;
  const float BurstTime=ManualFoilTime>=0?ManualFoilTime+(I<40?0.f:.3f):(I<40?10.5f:11.55f);
  const float Age=StageTime-BurstTime-R.FRandRange(0.f,.48f);
  const float OriginX=Side*R.FRandRange(172.f,234.f),OriginY=R.FRandRange(-130.f,50.f);
  const float VX=Side*R.FRandRange(12.f,115.f),VY=R.FRandRange(-75.f,55.f),VZ=R.FRandRange(365.f,575.f);
  const float Fall=530.f,Z=12.f+Age*(VZ-.5f*Fall*Age);
  const bool Visible=Celebrating&&Age>=0.f&&Z>=1.f;
  if(Ribbon->IsVisible()!=Visible)Ribbon->SetVisibility(Visible);
  if(!Visible)continue;
  const float Flutter=FMath::Sin(Age*9+I)*7.f*FMath::Min(Age,1.f);
  Ribbon->SetRelativeLocation(FVector(OriginX+VX*Age+Flutter,OriginY+VY*Age,Z));
  const float Spin=R.FRandRange(135.f,395.f)*(I%3?1.f:-1.f);
  Ribbon->SetRelativeRotation(FRotator(I*37.f+Age*Spin, I*71.f-Age*180.f, I*23.f+Age*255.f));
 }
}
void ARescueWorld::UpdateDance(){
 if(Reduced)return;
 const float T=StageTime,Beat=T*(128.f/60.f)*2*PI,Sway=FMath::Sin(Beat*.5f),Step=FMath::Sin(Beat),Hop=FMath::Abs(FMath::Sin(Beat))*9;
 const float Finish=FMath::Max(14.f,DanceMoveUntil);
 const float MovePhase=DanceMove>=0?FMath::Clamp((T-DanceMoveStart)/1.75f,0.f,1.f):0.f;
 float Fade=FMath::Clamp(T/.45f,0.f,1.f)*FMath::Clamp((Finish-T)/.6f,0.f,1.f);
 FVector Center(0,0,310),Shift(Fade*(Sway*13+(T>10?FMath::Sin(Beat*.25f)*25:0)),0,-27+Hop*Fade);
 if(DanceMove==0)Shift.Z+=FMath::Square(FMath::Sin(MovePhase*PI*3))*33.f;
 const float Spin=DanceMove==2?360.f*FMath::SmoothStep(0.f,1.f,MovePhase):0.f;
 FRotator BodyRot(Sway*5*Fade,FMath::Sin(Beat*.5f)*9*Fade+Spin,Step*3*Fade);
 FQuat Q=BodyRot.Quaternion();
 for(int I=0;I<BodyMeshes.Num();I++){BodyMeshes[I]->SetRelativeLocation(Center+Q.RotateVector(BodyRest[I]-Center)+Shift);BodyMeshes[I]->SetRelativeRotation(Q*FRotator(0,180,0).Quaternion());}
 for(int I=0;I<9;I++){
  FRotator Local=FRotator::ZeroRotator;FVector Lift=FVector::ZeroVector,Joint=FVector::ZeroVector;
  if(I<2){float Side=I==0?-1.f:1.f;Joint=FVector(0,0,25);
   // Crossed reins, then one raised lasso arm; both shoulders remain attached.
   float Cross=-Side*(57+FMath::Sin(Beat*2)*8);
   if((DanceMove==1||(DanceMove<0&&T>=6&&T<10))&&I==1){Local=FRotator(150+FMath::Sin(Beat)*22,FMath::Cos(Beat)*22,15*FMath::Sin(Beat));}
   else if(DanceMove==2||(DanceMove<0&&T>=10&&T<13)){Local=FRotator(Side*(65+FMath::Sin(Beat)*20),Side*12,Step*10);}
   else if(DanceMove<0&&T>=13){Local=FRotator(Side*120,0,0);}
   else Local=FRotator(Cross,Side*18,50+FMath::Sin(Beat*2)*10);
  }else if(I<4){float S=I==2?1.f:-1.f;float Knee=FMath::Max(0.f,Step*S);Joint=FVector(0,0,10);Local=FRotator(S*9+S*Knee*18,Knee*S*13,-Knee*22);Lift=FVector::ZeroVector;
  }else if(I==8){Local=FRotator(FMath::Sin(Beat)*7,-Sway*13,0);}
  Local.Pitch*=Fade;Local.Yaw*=Fade;Local.Roll*=Fade;FQuat Pose=Q*Local.Quaternion();
  auto* P=Parts[I];FVector Socket=Center+Q.RotateVector(P->Socket-Center)+Shift;
  P->SetActorLocation(Socket+Q.RotateVector(Joint)-Pose.RotateVector(Joint)+Lift);P->SetActorRotation(Pose);
 }
}
void ARescueWorld::UpdateLever(){if(LeverStick){FRotator Angle(FMath::Lerp(-24.f,55.f,Lever),0,0);FVector Pivot(-365,-80,77),Axis=Angle.RotateVector(FVector::UpVector);LeverStick->SetRelativeRotation(Angle);LeverStick->SetRelativeLocation(Pivot+Axis*50);LeverKnob->SetRelativeLocation(Pivot+Axis*100);}}
void ARescueWorld::Release(){if(Stage!=7||!Ready()){Say(TEXT("We still have repairs to do. The lever will turn green when I'm ready."));Beep(200);return;}Stage=8;Selected=-1;Lever=1;Refresh();YayStarted=false;if(Cheer){auto* Alive=LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/ComingAlive.ComingAlive"));auto* Yay=LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/Yay.Yay"));AliveDuration=Alive?Alive->GetDuration():0;CheerDuration=Yay?Yay->GetDuration():0;Voice->Stop();Effects->Stop();Cheer->SetSound(Alive);Cheer->SetVolumeMultiplier(Quiet?0.f:1.f);Cheer->Play();}Say(TEXT("Release sequence ready. Feet on the platform."));}
FVector2D ARescueWorld::Screen(FVector P)const{FVector2D Q=FVector2D::ZeroVector;auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return Q;PC->ProjectWorldLocationToScreen(P,Q);int X,Y;PC->GetViewportSize(X,Y);float S=FMath::Min(X/1600.f,Y/900.f);return (Q-FVector2D((X-1600*S)/2,(Y-900*S)/2))/FMath::Max(S,.01f);}
FVector ARescueWorld::World(FVector2D P)const{auto* PC=GetWorld()->GetFirstPlayerController();int X,Y;PC->GetViewportSize(X,Y);float S=FMath::Min(X/1600.f,Y/900.f);FVector O,D;PC->DeprojectScreenPositionToWorld(P.X*S+(X-1600*S)/2,P.Y*S+(Y-900*S)/2,O,D);float T=(-60-O.Y)/D.Y;return O+D*T;}
int32 ARescueWorld::HitPart(FVector2D P)const{int Best=-1;float Dist=65;for(int I=0;I<9;I++)if(Available(I)){float D=FVector2D::Distance(P,Screen(Parts[I]->GetActorTransform().TransformPosition(FVector(0,0,I<4?-55:0))));if(D<Dist){Dist=D;Best=I;}}return Best;}

int32 ARescueWorld::HitSocket(FVector2D P)const{for(int I=0;I<9;I++)if(Available(I)&&FVector2D::Distance(P,Screen(Parts[I]->Socket))<60)return I;return -1;}
FVector2D ARescueWorld::Terminal(int32 I)const{const FVector2D P[]={FVector2D(775,370),FVector2D(775,535),FVector2D(1200,345),FVector2D(1200,415),FVector2D(1200,495),FVector2D(1200,565)};return P[FMath::Clamp(I,0,5)];}
int32 ARescueWorld::HitTerminal(FVector2D P)const{for(int I=0;I<6;I++)if(FVector2D::Distance(P,Terminal(I))<34)return I;return -1;}
static bool In(FVector2D P,float X,float Y,float W,float H){return P.X>=X&&P.X<=X+W&&P.Y>=Y&&P.Y<=Y+H;}
void ARescueWorld::Click(FVector2D P){
 Mouse=P;
 if(In(P,35,25,80,80)){Settings=!Settings;Paused=Settings;return;}
 if(Settings){if(In(P,570,680,460,48)){ReturnToMenu();return;}if(In(P,570,340,460,50)){Quiet=!Quiet;if(Quiet)Voice->Stop();Save();}if(In(P,570,410,460,50)){Reduced=!Reduced;Save();}if(In(P,570,480,460,50)){Help=!Help;Settings=false;Paused=false;GiveHint();}if(In(P,570,550,460,50)){Settings=false;Paused=false;}if(In(P,570,610,225,48))ChooseChallenge(false);if(In(P,805,610,225,48))ChooseChallenge(true);return;}
 if(Stage==0){if(!Paused)MenuClick(P);return;}
 if(In(P,35,25,120,45)){Paused=!Paused;return;}if(Paused)return;
 if(In(P,1240,25,170,45)){Help=!Help;Say(Task);return;}
 if(CarMode){CarClick(P);return;}
 if(ClickWorkshop(P))return;
 if(Stage==8||Stage==9){if(In(P,1340,730,220,48))Complete();return;}
 if(Stage<7&&LeverKnob&&FVector2D::Distance(P,Screen(LeverKnob->GetComponentLocation()))<40){Say(TEXT("The lever is locked until every repair is complete."));return;}
 if(Stage==7&&(In(P,1250,590,300,150)||(LeverKnob&&FVector2D::Distance(P,Screen(LeverKnob->GetComponentLocation()))<50))){LeverDrag=true;LeverStartY=P.Y;Lever=0;Say(TEXT("Pull down, or press Enter to release."));return;}
 if(Stage==5){
  if(StageTime<.7f)return;
  if(In(P,970,160,186,48)){ChooseChallenge(false);return;}
  if(In(P,1160,160,202,48)){ChooseChallenge(true);return;}
  if(In(P,240,690,190,48)||In(P,445,690,190,48)){WireColor=P.X<440?0:1;if(ActiveTrack/2!=WireColor)ActiveTrack=WireColor*2;Eraser=false;WireStart=-1;PenDown=false;return;}
  if(In(P,650,690,130,48)){Eraser=!Eraser;WireStart=-1;return;}
  if(In(P,795,690,130,48)){UndoTrack();return;}
  if(In(P,940,690,185,48)){TestCircuit();return;}if(In(P,1140,690,220,48)){SendCircuit();return;}
  if(Eraser&&In(P,720,290,640,330)){int C=ActiveTrack;float Best=40;for(int J=0;J<4;J++)for(int K=1;K<Tracks[J].Num();K++){float D=PointSegment(P,Tracks[J][K-1],Tracks[J][K]);if(D<Best){Best=D;C=J;}}ActiveTrack=C;PreviousTracks[C]=Tracks[C];PreviousStart[C]=TrackStart[C];PreviousEnd[C]=TrackEnd[C];Tracks[C].Empty();TrackStart[C]=TrackEnd[C]=-1;CircuitPassed=false;TestStarted=-100;TracksTouch=false;return;}
  int T=HitTerminal(P);if(T>=0){if(WireStart>=0&&(T>=2&&T<=5)){FinishTrack(T);return;}if(T<2){WireColor=T;WireStart=T;ActiveTrack=T*2;PenPoints={Terminal(T)};PenDown=true;}else MonitorMessage=TEXT("Start at the red + or black - supply terminal.");}
  else if(Challenge&&WireStart>=0&&In(P,735,310,610,290))AddRouteWaypoint(P);
  return;
 }
 if(Stage==4&&Selected==7&&In(P,1220,650,310,50)){BatteryBackward=!BatteryBackward;Parts[7]->SetActorRotation(FRotator(BatteryBackward?180:0,0,0));Say(TEXT("Battery turned. Match the polarity symbols."));return;}
 int H=HitPart(P);if(H>=0){Pick(H);Dragging=true;return;}
 if(Selected>=0){int S=HitSocket(P);if(S>=0)TryPlace(Selected,S);else {Dragging=false;Parts[Selected]->SetActorLocation(Parts[Selected]->Home);}}
}
void ARescueWorld::Up(FVector2D P){
 Mouse=P;if(Paused)return;
 if(CarMode&&LeverDrag){LeverDrag=false;if(P.Y>LeverStartY+65)ReleaseCar();else Lever=0;return;}
 if(CarMode&&CarWiring&&WireStart>=0){PenDown=false;int T=HitTerminal(P);if(T>=2&&T<=5)FinishTrack(T);else if(Challenge&&In(P,735,310,610,290)&&FVector2D::Distance(P,Terminal(WireStart))>45)AddRouteWaypoint(P);else if(FVector2D::Distance(P,Terminal(WireStart))>45){WireStart=-1;PenPoints.Empty();}return;}
 if(CarMode){CarUp(P);return;}
 if(LeverDrag){LeverDrag=false;if(P.Y>LeverStartY+65)Release();else {Lever=0;Say(TEXT("Pull the lever all the way down."));}return;}
 if(Stage==5&&WireStart>=0){PenDown=false;int T=HitTerminal(P);if(T>=2&&T<=5)FinishTrack(T);else if(Challenge&&In(P,735,310,610,290)&&FVector2D::Distance(P,Terminal(WireStart))>45)AddRouteWaypoint(P);else if(FVector2D::Distance(P,Terminal(WireStart))>45){WireStart=-1;PenPoints.Empty();MonitorMessage=TEXT("Finish on a matching gold pad. Your earlier tracks are safe.");}return;}
 if(Dragging&&Selected>=0){int H=HitSocket(P);if(H>=0){TryPlace(Selected,H);}else if(FVector2D::Distance(P,Screen(Parts[Selected]->Home))>65){Failures++;Say(TEXT("No problem. Try the matching outlined socket."));}Dragging=false;if(Selected>=0)Parts[Selected]->SetActorLocation(Parts[Selected]->Home);}
}
void ARescueWorld::Key(FKey K){
 if(Stage==0&&NameEntry){FString S=K.ToString();if(K==EKeys::BackSpace&&Nickname.Len())Nickname.LeftChopInline(1);else if(K==EKeys::Enter){NameEntry=false;Save();}else if(S.Len()==1&&FChar::IsAlpha(S[0])&&Nickname.Len()<16)Nickname+=S;else if(K==EKeys::SpaceBar&&Nickname.Len()<16)Nickname+=TEXT(" ");return;}
 if(K==EKeys::Escape&&Stage==0&&!Settings){MenuPage=0;PendingLevel=0;MenuLoadTime=-1;NameEntry=false;return;}
 if(K==EKeys::Escape){if(Settings){Settings=false;Paused=false;}else Paused=!Paused;return;}
 if(CarMode&&CarReleased&&K==EKeys::H){CarKey(K);return;}
 if(K==EKeys::H){if(Stage==5)ChooseChallenge(false);else{Help=!Help;GiveHint();}return;}if(K==EKeys::M){Quiet=!Quiet;if(Quiet)Voice->Stop();Save();return;}
 if(K==EKeys::F2){Reduced=!Reduced;Save();return;}
 if(K==EKeys::F1){Settings=!Settings;Paused=Settings;return;}if(Paused)return;
 if((Stage==0||Stage==5)&&K==EKeys::F3){ChooseChallenge(!Challenge);return;}
 if(CarMode){CarKey(K);return;}
 if(KeyWorkshop(K))return;
 if(Stage==0){
  if(MenuPage==2)return;
  if(K==EKeys::N){NameEntry=true;return;}
  if(K==EKeys::Left||K==EKeys::Right||K==EKeys::Tab)MenuSelection=MenuSelection==1?2:1;
  if(K==EKeys::One)MenuSelection=1;if(K==EKeys::Two)MenuSelection=2;
  if(K==EKeys::Enter||K==EKeys::SpaceBar){if(MenuPage==0)MenuPage=1;else RequestLevel(MenuSelection);}
  return;
 }
 if(K==EKeys::R&&Stage==4&&Selected==7){BatteryBackward=!BatteryBackward;Parts[7]->SetActorRotation(FRotator(BatteryBackward?180:0,0,0));return;}
 if(K==EKeys::Tab||K==EKeys::Right||K==EKeys::Left){
  if(Stage==5){Focus=(Focus+(K==EKeys::Left?5:1))%6;return;}
  int Start=Selected;for(int J=1;J<=9;J++){int I=(Start+J+9)%9;if(Available(I)){Pick(I);break;}}return;
 }
 if(Stage==5&&K==EKeys::W&&Challenge&&WireStart>=0){
  AddRouteWaypoint(FVector2D(919,315));AddRouteWaypoint(FVector2D(1068,315));return;
 }
 if(Stage==5){if(K==EKeys::One||K==EKeys::Two){WireColor=K==EKeys::One?0:1;if(ActiveTrack/2!=WireColor)ActiveTrack=WireColor*2;Eraser=false;WireStart=-1;}if(K==EKeys::Z)UndoTrack();if(K==EKeys::T)TestCircuit();if(K==EKeys::S)SendCircuit();if(K==EKeys::Enter||K==EKeys::SpaceBar){if(WireStart<0){if(Focus<2){WireColor=Focus;WireStart=Focus;ActiveTrack=Focus*2;PenPoints={Terminal(Focus)};}}else if(Focus>=2&&Focus<=5)FinishTrack(Focus);}return;}
 if(K==EKeys::Enter||K==EKeys::SpaceBar){if(Stage==7){Release();return;}if(Stage==8||Stage==9){Complete();return;}if(Stage==10){if(Collection)Collection=false;else Reset();return;}if(Selected>=0){TryPlace(Selected,Selected);}else for(int I=0;I<9;I++)if(Available(I)){Pick(I);break;}return;}
 if(K==EKeys::C&&Stage==10)Collection=!Collection;
}
void ARescueWorld::Save(){
 if((FParse::Param(FCommandLine::Get(),TEXT("RRTest"))&&!TestSaving)||FParse::Param(FCommandLine::Get(),TEXT("RRNoSave")))return;
 FString S=FString::Printf(TEXT("[Progress]\nStage=%d\nRed=%d\nBlack=%d\nXP=%d\nFirstClear=%d\nNickname=%s\nQuiet=%d\nReduced=%d\n"),CarMode?CarReturnStage:Stage,Red,Black,XP,FirstClear,*Nickname,Quiet,Reduced);
 S+=FString::Printf(TEXT("Challenge=%d\nJobMedals=%d\nActiveRobot=%d\nCarInstalled=%d\nCarComplete=%d\n"),Challenge,JobMedals,ActiveRobot,CarInstalled,CarComplete);
 S+=FString::Printf(TEXT("CarEnginePlaced=%d\nCarEngineAligned=%d\nCarCylinders=%d\nCarWired=%d\nCarHead=%d\nCarReleased=%d\nCarPaintIndex=%d\nCarLook=%d\n"),CarEnginePlaced,CarEngineAligned,CarCylinders,CarWired,CarHead,CarReleased,CarPaintIndex,CarLook);
 S+=FString::Printf(TEXT("CarEverComplete=%d\n"),CarEverComplete);
 for(int I=0;I<3;I++)S+=FString::Printf(TEXT("Paint%d=%d\nFace%d=%d\nAccessory%d=%d\n"),I,RobotStyles[I].Paint,I,RobotStyles[I].Face,I,RobotStyles[I].Accessory);
 for(int I=0;I<Parts.Num();I++)S+=FString::Printf(TEXT("Part%d=%d\n"),I,Parts[I]->Installed);IFileManager::Get().MakeDirectory(*FPaths::ProjectSavedDir(),true);FFileHelper::SaveStringToFile(S,*(SaveFile+TEXT(".tmp")));IFileManager::Get().Move(*SaveFile,*(SaveFile+TEXT(".tmp")),true,true);
}
void ARescueWorld::Load(){
 if((FParse::Param(FCommandLine::Get(),TEXT("RRTest"))&&!TestSaving)||FParse::Param(FCommandLine::Get(),TEXT("RRNoSave")))return;
 FConfigFile C;if(!FPaths::FileExists(SaveFile))return;C.Read(SaveFile);C.GetInt(TEXT("Progress"),TEXT("Stage"),Stage);Stage=FMath::Clamp(Stage,0,10);C.GetBool(TEXT("Progress"),TEXT("Red"),Red);C.GetBool(TEXT("Progress"),TEXT("Black"),Black);C.GetInt(TEXT("Progress"),TEXT("XP"),XP);C.GetBool(TEXT("Progress"),TEXT("FirstClear"),FirstClear);C.GetString(TEXT("Progress"),TEXT("Nickname"),Nickname);Nickname=Nickname.Left(16);C.GetBool(TEXT("Progress"),TEXT("Quiet"),Quiet);C.GetBool(TEXT("Progress"),TEXT("Reduced"),Reduced);
 C.GetBool(TEXT("Progress"),TEXT("Challenge"),Challenge);
 C.GetInt(TEXT("Progress"),TEXT("CarInstalled"),CarInstalled);CarInstalled&=31;
 C.GetBool(TEXT("Progress"),TEXT("CarComplete"),CarComplete);CarEverComplete=CarComplete;C.GetBool(TEXT("Progress"),TEXT("CarEverComplete"),CarEverComplete);
 C.GetBool(TEXT("Progress"),TEXT("CarEnginePlaced"),CarEnginePlaced);C.GetBool(TEXT("Progress"),TEXT("CarEngineAligned"),CarEngineAligned);C.GetInt(TEXT("Progress"),TEXT("CarCylinders"),CarCylinders);
 C.GetBool(TEXT("Progress"),TEXT("CarWired"),CarWired);C.GetBool(TEXT("Progress"),TEXT("CarHead"),CarHead);C.GetBool(TEXT("Progress"),TEXT("CarReleased"),CarReleased);C.GetInt(TEXT("Progress"),TEXT("CarPaintIndex"),CarPaintIndex);C.GetInt(TEXT("Progress"),TEXT("CarLook"),CarLook);
 CarCylinders=FMath::Clamp(CarCylinders,0,3);CarPaintIndex=FMath::Clamp(CarPaintIndex,0,3);CarLook=FMath::Clamp(CarLook,0,1);
 if(!CarEnginePlaced)CarCylinders=0;
 if(CarCylinders<3)CarInstalled&=15;
 if(!(CarInstalled&16))CarWired=false;
 if(!CarWired)CarHead=false;
 if(!CarHead){CarReleased=false;CarComplete=false;}
 C.GetInt(TEXT("Progress"),TEXT("JobMedals"),JobMedals);
 JobMedals&=3;
 C.GetInt(TEXT("Progress"),TEXT("ActiveRobot"),ActiveRobot);
 if(!RobotUnlocked(ActiveRobot))ActiveRobot=0;
 for(int I=0;I<3;I++){
  auto& Style=RobotStyles[I];
  C.GetInt(TEXT("Progress"),*FString::Printf(TEXT("Paint%d"),I),Style.Paint);
  C.GetInt(TEXT("Progress"),*FString::Printf(TEXT("Face%d"),I),Style.Face);
  C.GetInt(TEXT("Progress"),*FString::Printf(TEXT("Accessory%d"),I),Style.Accessory);
  Style.Paint=FMath::Clamp(Style.Paint,0,3);Style.Face=FMath::Clamp(Style.Face,0,2);Style.Accessory=FMath::Clamp(Style.Accessory,0,2);
 }
 for(int I=0;I<9;I++){bool B=false;C.GetBool(TEXT("Progress"),*FString::Printf(TEXT("Part%d"),I),B);Parts[I]->Installed=B;}
 // Derive the next playable state from committed parts, repairing inconsistent saves.
 if(Stage>0&&Stage<10){Stage=1;bool Limbs=true;for(int I=0;I<4;I++)Limbs&=Parts[I]->Installed;if(Limbs){Stage=2;if(Parts[4]->Installed&&Parts[5]->Installed){Stage=3;if(Parts[6]->Installed){Stage=4;if(Parts[7]->Installed){Stage=5;if(Red&&Black){Stage=6;if(Parts[8]->Installed)Stage=7;}}}}}}
 if(Stage>0&&Stage<10){
  int32 ClearFrom=Stage==1?4:(Stage==2?6:(Stage==3?7:(Stage<=5?8:9)));
  for(int I=ClearFrom;I<9;I++)Parts[I]->Installed=false;
  if(Stage<5){Red=false;Black=false;}
 }
 if(Stage==10)for(auto* Chain:Chains)Chain->SetVisibility(false);
 Say(Stage==0?TEXT("A powerful machine. A friend waiting for you."):TEXT("Welcome back, engineer. Let's finish this rescue."));
}
void ARescueWorld::Complete(){if(Stage!=8&&Stage!=9)return;AwakenTime=-1;CircuitPassed=false;TestStarted=-100;PenDown=false;Eraser=false;TracksTouch=false;ActiveTrack=0;for(int C=0;C<4;C++){Tracks[C].Empty();PreviousTracks[C].Empty();TrackStart[C]=TrackEnd[C]=PreviousStart[C]=PreviousEnd[C]=-1;}Cheer->Stop();Effects->Stop();YayStarted=false;Stage=10;for(auto* C:Chains)C->SetVisibility(false);if(!FirstClear){FirstClear=true;XP+=100;}Refresh();Save();Say(TEXT("Robot rescued! Junior Engineer unlocked. Welcome to the team!"),TEXT("Complete"));}
void ARescueWorld::Reset(){WorkshopJob=0;ActiveRobot=0;ReactionPart=-1;PowerCountdown=-1;AwakenTime=-1;CircuitPassed=false;TestStarted=-100;PenDown=false;Eraser=false;TracksTouch=false;ActiveTrack=0;for(int C=0;C<4;C++){Tracks[C].Empty();PreviousTracks[C].Empty();TrackStart[C]=TrackEnd[C]=PreviousStart[C]=PreviousEnd[C]=-1;}Cheer->Stop();Effects->Stop();YayStarted=false;BatteryBackward=false;Stage=1;Red=Black=false;Selected=-1;WireStart=-1;Collection=false;Dragging=false;Lever=0;for(auto* P:Parts){P->Installed=false;P->SetActorRotation(FRotator::ZeroRotator);}for(auto* C:Chains)C->SetVisibility(true);Refresh();Save();Say(TEXT("Let's rescue our friend again."));}
void ARescueWorld::Tick(float Dt){
 Super::Tick(Dt);Clock+=Dt;
 if(Stage==0&&MenuPage==2&&!Paused&&Clock>=MenuLoadTime)FinishLevelLoad();
 RecordPen(Mouse);
UpdateMusic();if(Voice)Voice->SetPaused(Paused);if(Cheer){Cheer->SetPaused(Paused);Cheer->SetVolumeMultiplier(Quiet?0.f:1.f);}if(ToneAudio)ToneAudio->SetPaused(Paused);if(Effects){Effects->SetPaused(Paused);Effects->SetVolumeMultiplier(Quiet?0.f:.85f);}
 FString ShotPath;if(!ShotTaken&&Clock>3&&FParse::Value(FCommandLine::Get(),TEXT("RRShot="),ShotPath)){FScreenshotRequest::RequestScreenshot(ShotPath,true,false);ShotTaken=true;}
 if(ShotTaken&&Clock>5&&FParse::Param(FCommandLine::Get(),TEXT("RRShotExit")))UGameplayStatics::GetPlayerController(this,0)->ConsoleCommand(TEXT("quit"));
 FString CarCapture;if(FParse::Value(FCommandLine::Get(),TEXT("RRCarCapture="),CarCapture)&&Clock>2){
  if(FXCaptureFrame<0){FXCaptureFrame=0;CarReleased=false;CarReleaseProgress=0;CarTravel=CarTravelTarget=0;ReleaseCar();}
  IFileManager::Get().MakeDirectory(*CarCapture,true);FScreenshotRequest::RequestScreenshot(CarCapture/FString::Printf(TEXT("zip-%04d.png"),FXCaptureFrame),true,false);FXCaptureFrame++;
  if(FXCaptureFrame>=360)UGameplayStatics::GetPlayerController(this,0)->ConsoleCommand(TEXT("quit"));
 }
 FString DanceDir;if(FParse::Value(FCommandLine::Get(),TEXT("RRDanceCapture="),DanceDir)&&Clock>2){
  if(FXCaptureFrame<0){FXCaptureFrame=0;Stage=9;Refresh();for(auto* C:Chains)C->SetVisibility(false);}
  IFileManager::Get().MakeDirectory(*DanceDir,true);FScreenshotRequest::RequestScreenshot(DanceDir/FString::Printf(TEXT("dance-%04d.png"),FXCaptureFrame),true,false);FXCaptureFrame++;
  if(FXCaptureFrame>419)UGameplayStatics::GetPlayerController(this,0)->ConsoleCommand(TEXT("quit"));
 }
 FString CaptureDir;if(FParse::Value(FCommandLine::Get(),TEXT("RRFXCapture="),CaptureDir)&&Clock>2){
  if(FXCaptureFrame<0){FXCaptureFrame=0;AwakenTime=0;Filaments.Empty();PlasmaSpawnClock=0;}
  IFileManager::Get().MakeDirectory(*CaptureDir,true);FScreenshotRequest::RequestScreenshot(CaptureDir/FString::Printf(TEXT("frame-%04d.png"),FXCaptureFrame),true,false);FXCaptureFrame++;
  if(FXCaptureFrame>115)UGameplayStatics::GetPlayerController(this,0)->ConsoleCommand(TEXT("quit"));
 }
 if(Paused)return;StageTime+=Dt;if(AwakenTime>=0){AwakenTime+=Dt;if(AwakenTime>3.8f)AwakenTime=-1;}if(FParse::Param(FCommandLine::Get(),TEXT("RRBurstPreview")))AwakenTime=1.1f;UpdateElectricity(Dt);
 float Drop=Stage>=9?-27.f:(Stage==8?-27.f*FMath::Clamp((StageTime-2.f)/.8f,0.f,1.f):0.f);
 for(int I=0;I<BodyMeshes.Num();I++){BodyMeshes[I]->SetRelativeLocation(BodyRest[I]+FVector(0,0,Drop));BodyMeshes[I]->SetRelativeRotation(FRotator(0,180,0));}
 for(auto* P:Parts)if(P->Installed)P->SetActorLocation(P->Socket+FVector(0,0,Drop));
 UpdateLever();
 FVector CamPos=CarMode?(CarReleased?FVector(0,-1200,325):FVector(0,-1020,355)):(Stage<=1?FVector(0,-1330,410):(Stage>=2&&Stage<=5?FVector(0,-940,400):FVector(0,-1350,470)));
 FVector Look=CarMode?(CarReleased?FVector(0,0,150):FVector(0,0,270)):(Stage<=1?FVector(0,0,310):(Stage>=2&&Stage<=5?FVector(0,0,310):FVector(0,0,315)));
 if(Camera){Camera->SetActorLocation(Reduced?CamPos:FMath::VInterpTo(Camera->GetActorLocation(),CamPos,Dt,3));Camera->SetActorRotation((Look-Camera->GetActorLocation()).Rotation());}
 if(Dragging&&Selected>=0)Parts[Selected]->SetActorLocation(World(Mouse));
 if(LeverDrag)Lever=FMath::Clamp((Mouse.Y-LeverStartY)/100.f,0.f,1.f);
 if(Stage==8){for(int I=0;I<Chains.Num();I++)if(StageTime>1+(I/24)*.4f)Chains[I]->SetVisibility(false);UpdateReleaseAudio();if(StageTime>FMath::Max(3.f,AliveDuration+CheerDuration)){Stage=9;Refresh();Say(Nickname.IsEmpty()?TEXT("I'm free! Thank you, Engineer!"):FString::Printf(TEXT("I'm free! Thank you, Engineer %s!"),*Nickname),TEXT("Free"));}}
 if(Stage==9){
  UpdateDanceControls();
  UpdateDance();
  if(StageTime>14&&DanceMove<0&&DanceQueue.IsEmpty()&&(ManualFoilTime<0||StageTime>=ManualFoilTime+2.35f))Complete();
 }
 UpdateDisco();
 UpdateFinaleFoil();
 TickCar(Dt);
 UpdateCarLevel();
 if(Stage>=3&&Stage!=9&&Parts[4]->Installed&&!Reduced){Parts[4]->SetActorRotation(FRotator(Clock*35,0,0));Parts[5]->SetActorRotation(FRotator(-Clock*51,0,0));}
 UpdateRepairReaction(Dt);
 if(FParse::Param(FCommandLine::Get(),TEXT("RRTest"))&&Clock>5){UGameplayStatics::GetPlayerController(this,0)->ConsoleCommand(TEXT("quit"));}
}

ARescueMode::ARescueMode(){HUDClass=ARescueHUD::StaticClass();PlayerControllerClass=ARescueController::StaticClass();DefaultPawnClass=nullptr;}
void ARescueMode::BeginPlay(){Super::BeginPlay();GetWorld()->SpawnActor<ARescueWorld>();}
void ARescueController::BeginPlay(){Super::BeginPlay();bShowMouseCursor=true;FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);}
ARescueWorld* ARescueController::FindWorld()const{for(TActorIterator<ARescueWorld> It(GetWorld());It;++It)return *It;return nullptr;}
FVector2D ARescueController::Pointer()const{float X=0,Y=0;GetMousePosition(X,Y);int VX,VY;GetViewportSize(VX,VY);float S=FMath::Min(VX/1600.f,VY/900.f);return FVector2D((X-(VX-1600*S)/2)/FMath::Max(.01f,S),(Y-(VY-900*S)/2)/FMath::Max(.01f,S));}
bool ARescueController::InputKey(const FInputKeyEventArgs& P){auto* W=FindWorld();if(W){if(P.Key==EKeys::LeftMouseButton){if(P.Event==IE_Pressed)W->Click(Pointer());if(P.Event==IE_Released)W->Up(Pointer());return true;}if(P.Event==IE_Pressed){W->Key(P.Key);return true;}}return Super::InputKey(P);}

bool ARescueController::InputTouch(const FTouchId TouchId,const ETouchType::Type Type,const FVector2D& P,const float Force,const uint64 Timestamp){
 auto* W=FindWorld();if(!W)return false;int X,Y;GetViewportSize(X,Y);float S=FMath::Min(X/1600.f,Y/900.f);FVector2D Q=(P-FVector2D((X-1600*S)/2,(Y-900*S)/2))/FMath::Max(.01f,S);
 if(Type==ETouchType::Began){Touching=true;W->Click(Q);}else if(Type==ETouchType::Ended){W->Up(Q);Touching=false;}else W->Mouse=Q;return true;
}

void ARescueHUD::Rect(float X,float Y,float Width,float Height,FLinearColor C){DrawRect(C,OX+X*Scale,OY+Y*Scale,Width*Scale,Height*Scale);}
void ARescueHUD::Text(FString S,float X,float Y,float Size,FLinearColor C){UFont* Font=GEngine->GetMediumFont();DrawText(S,C,OX+X*Scale,OY+Y*Scale,Font,Size/12.f*Scale,false);}
void ARescueHUD::Stroke(FVector2D A,FVector2D B,FLinearColor C,float T){DrawLine(OX+A.X*Scale,OY+A.Y*Scale,OX+B.X*Scale,OY+B.Y*Scale,C,T*Scale);}
void ARescueHUD::Ring(FVector2D P,float R,FLinearColor C,float T){for(int I=0;I<40;I++){float A=I*2*PI/40,B=(I+1)*2*PI/40;Stroke(P+FVector2D(FMath::Cos(A),FMath::Sin(A))*R,P+FVector2D(FMath::Cos(B),FMath::Sin(B))*R,C,T);}}
void ARescueHUD::Button(FString Label,float X,float Y,float Width,bool Active){
 bool Hover=W&&In(W->Mouse,X,Y,Width,48);bool Red=Label.Contains(TEXT("RED +")),Black=Label.Contains(TEXT("BLACK -")),Test=Label==TEXT("TEST CIRCUIT"),Send=Label==TEXT("SEND TO ROBOT");
 FLinearColor Face=Red?FLinearColor(.60f,.025f,.015f):(Black?FLinearColor(.012f,.014f,.018f):(Send?FLinearColor(.008f,.20f,.33f):FLinearColor(.07f,.105f,.12f)));
 FLinearColor Rim=Send?FLinearColor(.03f,.8f,1.f):(Red?FLinearColor(1.f,.20f,.10f):FLinearColor(.40f,.48f,.51f));
 auto Round=[&](float PX,float PY,float PW,float PH,float R,FLinearColor C){for(int I=0;I<FMath::CeilToInt(PH);I++){float DY=FMath::Max(0.f,FMath::Abs(I-PH*.5f)-(PH*.5f-R));float Inset=DY>0?R-FMath::Sqrt(FMath::Max(0.f,R*R-DY*DY)):0;Rect(PX+Inset,PY+I,PW-Inset*2,1.2f,C);}};
 auto Disk=[&](FVector2D P,float R,FLinearColor C){Round(P.X-R,P.Y-R,R*2,R*2,R,C);};
 if(Send){for(int I=8;I>0;I-=2)Round(X-I,Y-I,Width+I*2,48+I*2,10+I,FLinearColor(.01f,.65f,1.f,(Active?.045f:.018f)));}
 Round(X,Y+5,Width,48,8,FLinearColor(0,0,0,.75f));Round(X,Y,Width,48,8,Rim);Round(X+2,Y+2,Width-4,44,6,FLinearColor(.015f,.022f,.025f));
 for(int I=0;I<39;I++){float Gain=1.25f-I*.013f+(Hover?.20f:0.f);float Inset=I<4?5-I*.5f:3;Rect(X+Inset,Y+4+I,Width-Inset*2,1.2f,Face*Gain);}
 Stroke(FVector2D(X+9,Y+4),FVector2D(X+Width-9,Y+4),FLinearColor(1,1,1,.24f),1);
 Stroke(FVector2D(X+8,Y+43),FVector2D(X+Width-8,Y+43),FLinearColor(0,0,0,.6f),2);
 if(Test){FVector2D P(X+29,Y+24);Disk(P+FVector2D(0,3),27,FLinearColor(0,0,0,.8f));Disk(P,27,FLinearColor(.28f,.30f,.31f));Disk(P,25,FLinearColor(.87f,.91f,.92f));Disk(P,22,FLinearColor(.35f,.006f,.004f));
  for(int I=0;I<40;I++){float DY=I-20.f,Half=FMath::Sqrt(FMath::Max(0.f,400-DY*DY));Rect(P.X-Half,P.Y+DY,Half*2,1.2f,FLinearColor(.94f-I*.009f,.025f,.016f));}
  Round(P.X-13,P.Y-16,26,9,4,FLinearColor(1,.8f,.75f,.5f));Text(TEXT("TEST"),P.X-16,P.Y-6,11,White);Text(TEXT("CIRCUIT"),X+66,Y+15,16,White);return;
 }
 if(Red||Black){Ring(FVector2D(X+21,Y+24),7,Red?FLinearColor(1,.4f,.25f):Silver,2);}
 float Size=FMath::Min(16.f,(Width-(Red||Black?49:24))/(FMath::Max(1,Label.Len())*.58f));float TX=X+(Red||Black?38:14);
 Text(Label,TX,Y+15,Size,Send&&!Active?FLinearColor(.48f,.69f,.75f):White);
 if(Send&&Active){Ring(FVector2D(X+Width-17,Y+24),4,Cyan,3);}
}

void ARescueHUD::Cable(FVector2D A,FVector2D B,FLinearColor C){
 for(int Layer=0;Layer<2;Layer++){FVector2D Prev=A;for(int I=1;I<=30;I++){float T=I/30.f;FVector2D Q=FMath::Lerp(A,B,T)+FVector2D(0,FMath::Sin(T*PI)*35);Stroke(Prev,Q,Layer?C:FLinearColor(.40f,.48f,.54f),Layer?5:8);Prev=Q;}}
}

void ARescueWorld::AddFilament(FVector Start,FVector End,float Width,float Power,bool Branches){
 FRandomStream R(PlasmaSeed++*9173);FRescueFilament Arc;Arc.Points={Start,End};Arc.Duration=R.FRandRange(.14f,.28f);Arc.Life=Arc.Duration;Arc.Width=Width;Arc.Power=Power;
 float Amount=FVector::Distance(Start,End)*.18f;
 for(int Level=0;Level<6;Level++){TArray<FVector> Split;Split.Add(Arc.Points[0]);for(int I=1;I<Arc.Points.Num();I++){FVector Mid=(Arc.Points[I-1]+Arc.Points[I])*.5f;Mid+=FVector(R.FRandRange(-Amount,Amount),R.FRandRange(-Amount*.23f,Amount*.23f),R.FRandRange(-Amount,Amount));Split.Add(Mid);Split.Add(Arc.Points[I]);}Arc.Points=MoveTemp(Split);Amount*=.51f;}
 Filaments.Add(Arc);
 if(Branches){for(int B=0;B<2;B++){int At=R.RandRange(14,47);FVector Tip=Arc.Points[At]+FVector(R.FRandRange(-60,60),R.FRandRange(-12,6),R.FRandRange(-45,45));AddFilament(Arc.Points[At],Tip,Width*.42f,Power*.42f,false);}}
}
void ARescueWorld::UpdateElectricity(float Dt){
 if(!Plasma||!PlasmaMaterial||!PlasmaLight)return;
 bool Active=AwakenTime>=0&&AwakenTime<3.8f;float Envelope=Active?FMath::Clamp(AwakenTime/.12f,0.f,1.f)*FMath::Clamp((3.8f-AwakenTime)/.9f,0.f,1.f):0;
 PlasmaLight->SetIntensity((Reduced?180.f:1250.f)*Envelope);
 if(!Active||Reduced){Plasma->SetVisibility(false);Filaments.Empty();PlasmaSpawnClock=0;return;}
 Plasma->SetVisibility(true);PlasmaMaterial->SetScalarParameterValue(TEXT("Intensity"),Envelope*.8f);
 for(int I=Filaments.Num()-1;I>=0;I--){Filaments[I].Life-=Dt;if(Filaments[I].Life<=0)Filaments.RemoveAt(I);}
 PlasmaSpawnClock-=Dt;
 if(PlasmaSpawnClock<=0){
  PlasmaSpawnClock=.075f;FRandomStream R(PlasmaSeed*4729);
  const FVector Nodes[]={FVector(-93,-100,374),FVector(93,-100,374),FVector(0,-100,420),FVector(0,-108,325),FVector(-57,-96,224),FVector(57,-96,224),FVector(-130,-60,530),FVector(130,-60,530),FVector(0,-100,465)};
  int A=R.RandRange(0,5),B=R.RandRange(0,8);if(B==A)B=(B+3)%9;
  AddFilament(Nodes[A],Nodes[B],R.FRandRange(4.f,6.5f),1,true);
  if(Filaments.Num()<4)AddFilament(Nodes[3],Nodes[R.RandRange(0,2)],4,.8f,true);
 }
 TArray<FVector> V,N;TArray<int32> Tris;TArray<FVector2D> UV;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
 for(const auto& Arc:Filaments){int Base=V.Num();int Count=Arc.Points.Num();float Fade=FMath::Clamp(Arc.Life/.05f,0.f,1.f)*FMath::Clamp((Arc.Duration-Arc.Life)/.018f,0.f,1.f);
  for(int I=0;I<Count;I++){FVector P=Arc.Points[I];FVector Dir=(Arc.Points[FMath::Min(I+1,Count-1)]-Arc.Points[FMath::Max(0,I-1)]).GetSafeNormal();FVector Facing=(Camera->GetActorLocation()-P).GetSafeNormal();FVector Side=FVector::CrossProduct(Dir,Facing).GetSafeNormal()*Arc.Width*.5f;float U=I/(float)(Count-1);float EndFade=FMath::Pow(FMath::Sin(U*PI),.2f);
   for(int J=0;J<2;J++){V.Add(P+(J?Side:-Side));N.Add(Facing);UV.Add(FVector2D(U,J));Colors.Add(FLinearColor(1,1,1,Arc.Power*Fade*EndFade));Tangents.Add(FProcMeshTangent(Dir,false));}
   if(I>0){int Q=Base+I*2;Tris.Append({Q-2,Q-1,Q,Q-1,Q+1,Q});}
  }
 }
 Plasma->CreateMeshSection_LinearColor(0,V,Tris,N,UV,Colors,Tangents,false,false);
}
void ARescueHUD::DrawAwakening(){
 // Electricity is rendered in the 3D scene with emissive filaments and real bloom.
}

void ARescueHUD::DrawHUD(){
 Super::DrawHUD();if(!W)for(TActorIterator<ARescueWorld> It(GetWorld());It;++It){W=*It;break;}if(!W||!W->Parts.Num())return;
 Scale=FMath::Min(Canvas->SizeX/1600.f,Canvas->SizeY/900.f);OX=(Canvas->SizeX-1600*Scale)/2;OY=(Canvas->SizeY-900*Scale)/2;
 auto* PC=Cast<ARescueController>(PlayerOwner);if(PC&&!PC->Touching){W->Mouse=PC->Pointer();}
 W->RecordPen(W->Mouse);
 DrawAwakening();
 Rect(35,25,80,80,FLinearColor(.01f,.025f,.035f,.9f));for(int E=0;E<4;E++){FVector2D A(35+(E==1||E==2?80:0),25+(E>=2?80:0)),B(35+(E==0||E==1?80:0),25+(E==1||E==2?80:0));Stroke(A,B,Silver,2);}
 FVector2D Gear(75,65);Ring(Gear,19,Gold,7);Ring(Gear,8,Gold,3);for(int I=0;I<8;I++){float A=I*PI/4;Stroke(Gear+FVector2D(FMath::Cos(A),FMath::Sin(A))*20,Gear+FVector2D(FMath::Cos(A),FMath::Sin(A))*28,Gold,9);}
 Button(W->CarMode?TEXT("LEVEL 2"):TEXT("LEVEL 1"),1440,25,125);
 if(W->Stage==0){DrawMainMenu(); }else{
  if(W->Stage>0&&W->Stage<7&&W->Stage!=5){for(int I=0;I<9;I++)if(W->Available(I)){
   FVector2D P=W->Screen(W->Parts[I]->Socket);bool Match=W->Selected==I;bool Hover=FVector2D::Distance(W->Mouse,P)<60;Ring(P,I<4?26:32,Match?(Hover?Green:Cyan):(Hover&&W->Selected>=0?Gold:FLinearColor(.2f,.48f,.55f,.8f)),Match?3:1);
   if(Match){Text(TEXT("FIT HERE"),P.X-35,P.Y-52,12,Cyan);}
   if((W->Selected==I||FVector2D::Distance(W->Mouse,W->Screen(W->Parts[I]->Home))<65)&&(!W->Dragging||W->Selected!=I)){auto Q=W->Screen(W->Parts[I]->Home+FVector(0,0,I<4?-25:0));Rect(Q.X-76,Q.Y+57,152,26,FLinearColor(.01f,.03f,.04f,.9f));Text(W->Names[I],Q.X-65,Q.Y+63,12,Match?Green:White);if(Match)Ring(Q,64,Cyan,2);}
  }}
  if(W->Stage==4){Text(TEXT("BATTERY"),680,600,14,Cyan);Text(TEXT("+ POSITIVE     - NEGATIVE"),630,625,16,White);Button(TEXT("TURN BATTERY  [R]"),1220,650,310,W->Selected==7);}
  if(W->Stage==5||W->CarWiring||(W->Stage==6&&W->StageTime<.65f)){
   float OldOY=OY;float Open=W->CarWiring?1.f:(W->Stage==6?1.f-FMath::Clamp(W->StageTime/.65f,0.f,1.f):(W->Reduced?1.f:FMath::Clamp(W->StageTime/.65f,0.f,1.f)));OY+=(1-Open)*(1-Open)*800*Scale;
   // Steel bezel, pedestal and inset screen create a dedicated diagnostic workstation.
   Rect(730,755,140,65,Dark);Rect(600,800,400,12,Steel);Rect(166,117,1280,665,FLinearColor(0,0,0,.65f));
   Rect(178,100,1244,675,FLinearColor(.065f,.068f,.065f));Rect(158,120,1284,635,FLinearColor(.065f,.068f,.065f));for(int X:{178,1422})for(int Y:{120,755})Ring(FVector2D(X,Y),16,Steel,12);
   for(int B=0;B<12;B++)Rect(172+B,115+B,1256-2*B,645-2*B,FLinearColor(.24f-B*.012f,.25f-B*.012f,.24f-B*.012f));Rect(190,135,1220,605,FLinearColor(.012f,.026f,.026f));
   for(int J=0;J<14;J++)Rect(540+J*19,749,11,3,Dark);Ring(FVector2D(1365,750),8,Gold,5);Ring(FVector2D(1330,750),4,Green,3);
   for(int X:{180,1420})for(int Y:{125,750}){Ring(FVector2D(X,Y),5,Silver,2);}
   Rect(210,155,1180,56,FLinearColor(.08f,.105f,.095f));Ring(FVector2D(237,182),5,Green,4);Text(TEXT("ROBOT REPAIR TERMINAL"),255,170,24,White);Button(TEXT("HELP ME [H]"),970,160,186,!W->Challenge);Button(TEXT("CHALLENGE [F3]"),1160,160,202,W->Challenge);
   Text(W->Nickname.IsEmpty()?TEXT("ENGINEER, RESTORE THE POWER"):TEXT("ENGINEER ")+W->Nickname.ToUpper()+TEXT(", RESTORE THE POWER"),240,230,18,Gold);
   Rect(235,275,430,330,FLinearColor(.05f,.075f,.07f));Text(TEXT("1  FOLLOW THE SCHEMATIC"),255,292,18,White);
   FLinearColor RedInk(.95f,.09f,.055f),BlackInk(.015f,.02f,.023f);
   Text(TEXT("SUPPLY"),260,345,15,Silver);Text(TEXT("CHIP"),545,345,15,Silver);Rect(530,375,80,170,Dark);
   Ring(FVector2D(285,405),18,RedInk,5);Text(TEXT("+"),277,392,22,White);Ring(FVector2D(285,515),18,Silver,5);Text(TEXT("-"),277,502,22,White);
   for(int I=0;I<4;I++){float Y=390+I*45;Stroke(FVector2D(305,I<2?405:515),FVector2D(530,Y),I<2?RedInk:Silver,3);Text(I<2?TEXT("A"):TEXT("B"),555,Y-10,18,White);}
   Text(TEXT("RED + TO BOTH A PADS"),265,560,14,RedInk);Text(TEXT("BLACK - TO BOTH B PADS"),265,582,14,Silver);
   Rect(710,275,650,330,FLinearColor(.035f,.16f,.105f));Text(TEXT("2  DRAW ON THE PCB"),735,292,18,White);
   for(int X=750;X<1340;X+=25)for(int Y=330;Y<590;Y+=25)Rect(X,Y,2,2,FLinearColor(.10f,.29f,.20f));
   if(W->Challenge){
    Rect(944,335,98,43,Dark);
    for(int X=951;X<1034;X+=13)Stroke({float(X),339},{float(X-7),375},Gold,4);
    Text(TEXT("KEEP CLEAR"),949,348,12,White);
   }
   Rect(1225,325,105,265,Dark);Rect(1231,331,93,253,FLinearColor(.06f,.065f,.06f));Text(TEXT("POWER"),1250,438,13,Silver);Text(TEXT("CHIP"),1257,459,13,Silver);for(int J=0;J<13;J++){Rect(1219,339+J*19,6,6,Gold);Rect(1330,339+J*19,6,6,Gold);}
   for(int C=0;C<4;C++){
    auto& P=W->Tracks[C];for(int I=1;I<P.Num();I++){Stroke(P[I-1],P[I],Gold,10);Stroke(P[I-1],P[I],C>=2?BlackInk:RedInk,6);}
    float Progress=(W->StageTime-W->TestStarted-C*.6f)/.6f;
    if(P.Num()>1&&Progress>=0&&Progress<1){float F=Progress*(P.Num()-1);int I=FMath::Min((int)F,P.Num()-2);Ring(FMath::Lerp(P[I],P[I+1],F-I),8,Cyan,5);}
   }
   if(W->WireStart>=0){auto& P=W->PenPoints;for(int I=1;I<P.Num();I++)Stroke(P[I-1],P[I],W->WireColor?Silver:RedInk,5);if(P.Num())Stroke(P.Last(),W->Mouse,W->WireColor?Silver:RedInk,4);}
   for(int I=0;I<6;I++){auto P=W->Terminal(I);Ring(P,18,Gold,5);Rect(P.X-11,P.Y-11,22,22,Dark);Text(I==0?TEXT("+"):(I==1?TEXT("-"):(I<4?TEXT("A"):TEXT("B"))),P.X-7,P.Y-11,18,White);if(W->Focus==I)Ring(P,25,Cyan,2);}
   Text(TEXT("SUPPLY + / -"),735,585,12,Silver);
   if((W->TracksTouch||W->CircuitBlocked)&&W->TestStarted>=0)Ring(W->TouchPoint,18,Gold,3);
   if(!W->Challenge){
    const int Pad=W->GuidedPad();
    if(Pad>=0){
     const float Radius=29.f+(W->Reduced?0.f:2.f*FMath::Sin(W->Clock*4.f));
     Ring(W->Terminal(Pad),Radius,Green,3);
     Ring(W->Terminal(Pad<4?0:1),Radius,Green,3);
    }
   }
   FString Status=W->CircuitPassed&&W->StageTime-W->TestStarted>=2.4f?TEXT("CIRCUIT READY! Send your design to the robot."):W->MonitorMessage;Text(Status,240,625,17,W->CircuitPassed?Green:White);
   Text(W->Challenge?TEXT("Draw or tap waypoints around the guard. [W] Keyboard detour. [T] Test  [S] Send  [Z] Undo"):TEXT("Tap two glowing pads or draw a wire. [T] Test  [S] Send  [Z] Undo"),240,655,13,Silver);
   Button(W->WireColor==0&&!W->Eraser?TEXT("[1] RED +  *"):TEXT("[1] RED +"),240,690,190);Button(W->WireColor==1&&!W->Eraser?TEXT("[2] BLACK -  *"):TEXT("[2] BLACK -"),445,690,190);Button(W->Eraser?TEXT("ERASER *"):TEXT("ERASER"),650,690,130);Button(TEXT("UNDO [Z]"),795,690,130);Button(TEXT("TEST CIRCUIT"),940,690,185);Button(TEXT("SEND TO ROBOT"),1140,690,220,W->CircuitPassed&&W->StageTime-W->TestStarted>=2.4f);
   for(int Y=140;Y<615;Y+=4)Rect(195,Y,1210,1,FLinearColor(0,0,0,.035f));Stroke(FVector2D(205,142),FVector2D(1390,142),FLinearColor(.6f,.8f,.8f,.18f),2);
   OY=OldOY;
  }
  if(!W->CarMode&&W->Stage>=6&&W->Red&&W->Black){
   Cable(W->Screen(FVector(-25,-85,262)),W->Screen(FVector(-40,-85,288)),W->Stage==6&&W->StageTime<1.2f?Cyan:FLinearColor(.95f,.05f,.03f));
   Cable(W->Screen(FVector(25,-85,262)),W->Screen(FVector(40,-85,288)),W->Stage==6&&W->StageTime<1.2f?Cyan:FLinearColor(.01f,.015f,.02f));
  }
  if(W->Stage==7){auto P=W->Screen(W->LeverKnob->GetComponentLocation());Ring(P,24,Green,2);Text(TEXT("PULL TO RELEASE"),P.X-75,P.Y-50,13,Green);}
  if(W->Stage==8||W->Stage==9){
   Button(TEXT("SKIP / ENTER"),1340,730,220);
   if(W->Stage==9){Text(TEXT("I'M FREE!"),620,155,46,Gold);DrawDanceControls();}
  }
  if(W->Stage==6&&W->PowerCountdown>=0){
   Rect(630,675,340,78,FLinearColor(.01f,.03f,.04f,.94f));
   Text(FString::Printf(TEXT("SYSTEMS WAKE IN  %d"),W->PowerCount),665,699,23,Gold);
  }
  if(W->Stage<10||W->CarMode){Rect(250,816,1100,56,FLinearColor(.008f,.022f,.032f,.72f));FString Display=W->Line.IsEmpty()?W->Task:W->Line;Text(Display,275,834,Display.Len()>100?15:18,White);}
  if(W->Stage==10){if(W->CarMode&&!W->CarWiring)DrawCarLevel();else if(!W->CarMode)DrawWorkshop();}
 }
 if(W->Help&&W->Stage>0&&W->Stage<8&&W->Stage!=5){Rect(45,265,380,140,FLinearColor(.015f,.05f,.067f,.98f));Text(TEXT("ENGINEER'S HINT"),65,282,20,Cyan);Text(TEXT("Match the outline and the symbols."),65,325,16,White);Text(TEXT("Tab: choose. Enter: place or connect."),65,353,15,Silver);if(W->Stage==5)Text(TEXT("1: red +    2: black -"),65,379,15,Gold);}
 if(W->Paused&&!W->Settings){Rect(500,325,600,220,FLinearColor(.008f,.023f,.035f,.98f));Text(TEXT("WORKSHOP PAUSED"),560,375,32,White);Text(TEXT("Press Escape or RESUME to continue."),575,450,19,Cyan);}
 if(W->Settings){Rect(480,220,640,540,FLinearColor(.008f,.023f,.035f,.99f));Text(TEXT("WORKSHOP SETTINGS"),555,255,30,White);Button(W->Quiet?TEXT("SOUND: OFF  [M]"):TEXT("SOUND: ON  [M]"),570,340,460);Button(W->Reduced?TEXT("REDUCED MOTION: ON [F2]"):TEXT("REDUCED MOTION: OFF [F2]"),570,410,460);Button(TEXT("HINT / REPEAT GUIDANCE"),570,480,460);Button(TEXT("RESUME"),570,550,460);Button(TEXT("MAIN MENU"),570,680,460);Button(TEXT("HELP ME"),570,610,225,!W->Challenge);Button(TEXT("CHALLENGE ME"),805,610,225,W->Challenge);}
}

void ARescueWorld::RunTests(){
 TArray<FString> Checks;bool All=true;
 auto Check=[&](bool OK,const FString& Name){Checks.Add(FString::Printf(TEXT("    {\"name\":\"%s\",\"passed\":%s}"),*Name,OK?TEXT("true"):TEXT("false")));All&=OK;};
 Stage=1;for(auto* P:Parts)P->Installed=false;Red=Black=false;FirstClear=false;XP=0;Refresh();
 Click(FVector2D(75,65));Check(Settings&&Paused,TEXT("reference_gear_opens_settings_and_pauses"));Click(FVector2D(700,500));Check(Help&&!Settings&&!Paused,TEXT("guidance_remains_accessible_from_settings"));Help=false;
 Check(!TryPlace(0,1)&&!Parts[0]->Installed,TEXT("wrong_limb_rejected"));Release();Check(Stage==1,TEXT("early_lever_blocked"));
 TryPlace(1,1);Check(Effects->Sound&&Effects->Sound->GetName()==TEXT("LimbMechanism"),TEXT("successful_limb_plays_mechanism"));TryPlace(0,0);TryPlace(3,3);TryPlace(2,2);Check(Stage==2,TEXT("limbs_any_order_open_chest"));
 Check(!Available(8),TEXT("head_locked_until_wiring"));Check(!TryPlace(4,5),TEXT("wrong_gear_size_rejected"));TryPlace(4,4);Check(Effects->Sound&&Effects->Sound->GetName()==TEXT("GearAttach"),TEXT("gear_attachment_plays_user_audio"));TryPlace(5,5);Check(Stage==3,TEXT("gear_pair_stage_gate"));TryPlace(6,6);Check(Stage==4,TEXT("motor_stage_gate"));
 BatteryBackward=true;Parts[7]->SetActorRotation(FRotator(180,0,0));Check(!TryPlace(7,7),TEXT("backward_battery_rejected"));BatteryBackward=false;Parts[7]->SetActorRotation(FRotator::ZeroRotator);TryPlace(7,7);Check(Stage==5,TEXT("battery_stage_gate"));
 Check(!Connect(0,0,3)&&!Red,TEXT("cross_polarity_rejected"));Check(!Connect(1,0,2)&&!Black,TEXT("wrong_wire_type_rejected"));Connect(0,0,2);Check(Red&&Stage==5,TEXT("one_wire_not_enough"));Connect(1,1,2);Check(Red&&!Black,TEXT("valid_wire_survives_error"));Connect(1,1,3);Check(Stage==6,TEXT("two_wires_unlock_head"));Check(Effects->Sound&&Effects->Sound->GetName()==TEXT("MotorEffect"),TEXT("connected_battery_plays_motor"));
 TryPlace(8,8);Check(Effects->Sound&&Effects->Sound->GetName()==TEXT("HeadAttach"),TEXT("head_attachment_plays_user_audio"));Check(AwakenTime==0,TEXT("assembly_triggers_lightning_and_sparks"));Check(Ready()&&Stage==7,TEXT("all_components_ready"));
 AwakenTime=.3f;UpdateElectricity(.016f);Check(PlasmaMaterial&&Plasma->IsVisible()&&Filaments.Num()>0,TEXT("assembly_creates_emissive_3d_filaments"));Reduced=true;UpdateElectricity(.016f);Check(!Plasma->IsVisible()&&Filaments.Num()==0,TEXT("reduced_motion_suppresses_electrical_arcs"));Reduced=false;AwakenTime=-1;UpdateElectricity(.016f);Check(!Plasma->IsVisible(),TEXT("electrical_arcs_clean_up_after_effect"));
 Click(FVector2D(1350,620));Up(FVector2D(1350,650));Check(Stage==7&&!LeverDrag,TEXT("short_lever_drag_returns_safely"));
 Click(FVector2D(1350,620));Up(FVector2D(1350,730));Check(Stage==8,TEXT("full_pointer_lever_drag_releases"));
 Stage=7;Refresh();Release();Check(Stage==8,TEXT("valid_lever_releases"));
 for(int Pose=0;Pose<2;Pose++){Lever=Pose;UpdateLever();FVector Axis=LeverStick->GetRelativeRotation().RotateVector(FVector::UpVector);Check(FVector::Distance(LeverKnob->GetRelativeLocation(),LeverStick->GetRelativeLocation()+Axis*50)<.1f&&FVector::Distance(LeverStick->GetRelativeLocation()-Axis*50,FVector(-365,-80,77))<.1f,Pose?TEXT("lever_pulled_pose_stays_attached"):TEXT("lever_rest_pose_stays_attached"));}
 Check(Cheer->Sound&&Cheer->Sound->GetName()==TEXT("ComingAlive")&&!YayStarted,TEXT("lever_starts_coming_alive"));
 StageTime=FMath::Max(0.f,AliveDuration-.01f);UpdateReleaseAudio();Check(!YayStarted,TEXT("yay_waits_for_coming_alive"));
 StageTime=AliveDuration;UpdateReleaseAudio();Check(YayStarted&&Cheer->Sound&&Cheer->Sound->GetName()==TEXT("Yay"),TEXT("yay_follows_coming_alive"));
 Complete();Check(Stage==10&&XP==100&&FirstClear,TEXT("completion_rewards_once"));Complete();Check(XP==100,TEXT("duplicate_completion_no_reward"));Reset();Check(Stage==1&&XP==100&&FirstClear&&!Red&&!Black,TEXT("replay_preserves_earnings"));
 for(int I=0;I<4;I++)TryPlace(I,I);TryPlace(4,4);TryPlace(5,5);TryPlace(6,6);TryPlace(7,7);WireColor=0;WireStart=0;PenPoints.Empty();FinishTrack(3);TestCircuit();SendCircuit();Check(Stage==5&&!Red&&!Black&&!CircuitPassed,TEXT("monitor_wrong_draft_cannot_send"));UndoTrack();Check(Tracks[0].IsEmpty(),TEXT("monitor_undo_removes_track"));
 StageTime=1;Click(Terminal(0));RecordPen(FVector2D(900,350));RecordPen(FVector2D(1050,345));Up(Terminal(2));Check(Tracks[0].Num()>=4&&TrackStart[0]==0&&TrackEnd[0]==2,TEXT("pointer_red_drag_records_and_connects"));
 Click(Terminal(1));Up(Terminal(1));Click(Terminal(4));Up(Terminal(4));Check(TrackStart[2]==1&&TrackEnd[2]==4&&WireColor==1,TEXT("pointer_black_tap_autoselects_and_connects"));
 for(int C=0;C<4;C++){WireColor=C<2?0:1;WireStart=C<2?0:1;PenPoints.Empty();FinishTrack(C+2);}Check(!Red&&!Black&&Stage==5,TEXT("monitor_drawing_does_not_power_robot"));
 FVector2D Hit;Check(!TrackCollision(Hit),TEXT("four_separate_tracks_are_safe"));auto Safe=Tracks[1];Tracks[1]={FVector2D(1000,350),FVector2D(1100,390)};Tracks[0]={FVector2D(1000,390),FVector2D(1100,350)};TestCircuit();Check(TracksTouch&&!CircuitPassed,TEXT("crossing_tracks_block_test"));Tracks[1]={FVector2D(1000,356),FVector2D(1100,356)};Tracks[0]={FVector2D(1000,350),FVector2D(1100,350)};TestCircuit();Check(TracksTouch&&!CircuitPassed,TEXT("near_touching_tracks_block_test"));Tracks[1]=Safe;Tracks[0]={Terminal(0),Terminal(2)};TestCircuit();SendCircuit();Check(Stage==5&&CircuitPassed,TEXT("monitor_send_waits_for_test_pulse"));StageTime+=2.5f;SendCircuit();Check(Stage==6&&Red&&Black,TEXT("monitor_send_powers_robot_after_test"));TryPlace(8,8);Release();Complete();Check(XP==100,TEXT("full_replay_no_duplicate_xp"));
 Check(LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/ScienceMusic.ScienceMusic"))&&LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/Yay.Yay"))&&LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/PartyDance.PartyDance")),TEXT("all_user_audio_assets_load"));
 Stage=9;Refresh();Check(MusicStage==1&&Music->Sound&&Music->Sound->GetName()==TEXT("PartyDance"),TEXT("dance_uses_party_music"));
 bool WasReduced=Reduced;Reduced=false;StageTime=1;UpdateDance();auto Riding=Parts[1]->GetActorRotation();auto HeadA=Parts[8]->GetActorLocation();StageTime=7;UpdateDance();Check(!Parts[1]->GetActorRotation().Equals(Riding,10.f),TEXT("dance_lasso_differs_from_riding"));StageTime=1.2f;UpdateDance();Check(FVector::Distance(HeadA,Parts[8]->GetActorLocation())>1,TEXT("dance_moves_whole_robot"));Reduced=true;auto Still=Parts[1]->GetActorTransform();StageTime=8;UpdateDance();Check(Parts[1]->GetActorTransform().Equals(Still),TEXT("reduced_motion_suppresses_dance"));Reduced=WasReduced;

 Stage=8;Refresh();Check(MusicStage==2&&!Music->IsPlaying(),TEXT("release_stops_background_for_cheer"));
 Stage=10;Refresh();Check(MusicStage==0&&Music->Sound&&Music->Sound->GetName()==TEXT("ScienceMusic"),TEXT("background_resumes_after_dance"));
 TestSaving=true;SaveFile=FPaths::ProjectSavedDir()/TEXT("RobotRescueTestProgress.ini");
 Stage=8;Nickname=TEXT("TEST ENGINEER");Reduced=true;Save();Stage=1;for(auto* P:Parts)P->Installed=false;Red=Black=false;Load();
 Check(Stage==7&&Ready()&&XP==100,TEXT("interrupted_release_resumes_ready_without_duplicate_xp"));
 Check(Nickname==TEXT("TEST ENGINEER")&&Reduced,TEXT("profile_and_preferences_persist"));
 Parts[8]->Installed=false;Stage=5;Red=true;Black=false;Save();Stage=1;Red=false;Load();Check(Stage==5&&Red&&!Black,TEXT("partial_wiring_checkpoint_restores"));
 Parts[0]->Installed=false;Stage=9;Save();Load();Check(Stage==1,TEXT("inconsistent_checkpoint_recovers_to_earliest_stage"));TestSaving=false;
 // Verify the new interactions through the same inputs the workshop uses.
 Reduced=false;Stage=5;Refresh();StageTime=1;
 Check(GuidedPad()==2,TEXT("guided_mode_points_to_first_missing_pad"));
 Click({1250,181});
 for(int I=0;I<4;I++){WireStart=I<2?0:1;PenPoints.Empty();FinishTrack(I+2);}
 TestCircuit();
 Check(Challenge&&CircuitBlocked&&!CircuitPassed,TEXT("challenge_guard_blocks_straight_track"));
 WireStart=0;PenPoints={Terminal(0)};Key(EKeys::W);FinishTrack(2);TestCircuit();
 Check(CircuitPassed&&!CircuitBlocked,TEXT("keyboard_detour_clears_challenge_guard"));
 Stage=10;FirstClear=true;JobMedals=0;
 Click({200,385});Key(EKeys::Two);Key(EKeys::T);
 Check(WorkshopJob==1&&!JobPassed&&!RobotUnlocked(1),TEXT("heavier_delivery_rejects_weak_gearing"));
 Key(EKeys::Three);Key(EKeys::T);
 Check(JobPassed&&RobotUnlocked(1),TEXT("stronger_gearing_lifts_cargo_and_unlocks_parcel"));
 Key(EKeys::BackSpace);Click({200,448});
 Click({1135,454});Click({1285,368});Key(EKeys::T);
 Check(!JobPassed&&!RobotUnlocked(2),TEXT("reversed_pump_wire_does_not_water"));
 Click({1135,368});Click({1285,368});Click({1135,454});Click({1285,454});Key(EKeys::T);
 Check(JobPassed&&RobotUnlocked(2),TEXT("correct_pump_wiring_waters_and_unlocks_sprout"));
 for(auto* Part:Parts)Part->Installed=true;
 Red=Black=true;
 Key(EKeys::P);Key(EKeys::V);Key(EKeys::B);Key(EKeys::N);
 TestSaving=true;Save();ActiveRobot=0;JobMedals=0;RobotStyles[2]=FRescueStyle();Load();TestSaving=false;
 Check(Collection&&ActiveRobot==2&&JobMedals==3&&RobotStyles[2].Paint==3&&RobotStyles[2].Face==2&&RobotStyles[2].Accessory==1&&!StylePaint.IsEmpty()&&!StyleEyes.IsEmpty(),TEXT("robot_styles_and_unlocks_persist"));
 Key(EKeys::P);Key(EKeys::Two);Key(EKeys::Three);UpdateDanceControls();
 Check(Stage==9&&DanceMove==1&&DanceQueue.Num()==1&&DanceQueue[0]==2,TEXT("collection_replay_queues_dance_moves_in_order"));
 Key(EKeys::C);
 Check(ManualFoilTime==StageTime,TEXT("dance_player_triggers_confetti"));
 Stage=10;FirstClear=true;CarInstalled=0;CarComplete=false;
 Click({150,630});Check(CarMode,TEXT("level_two_unlocks_after_first_rescue"));
 FitCarPart(4);Check(CarInstalled==0,TEXT("car_battery_waits_for_wheels"));
 for(int I=0;I<4;I++){CarSelected=I;CarKey(EKeys::Enter);}
 Check((CarInstalled&15)==15,TEXT("four_car_wheels_fit"));
 FitCarPart(5);CarEngineClick({550,490});CarEngineClick({550,490});CarEngineClick({600,560});CarEngineClick({785,390});CarUp({1085,390});CarEngineClick({785,390});CarUp({935,390});
 FitCarPart(4);Challenge=false;for(int I=0;I<4;I++){WireStart=I<2?0:1;PenPoints.Empty();FinishTrack(I+2);}TestCircuit();StageTime+=2.5f;SendCircuit();
 FitCarPart(6);CarKey(EKeys::L);Check(CarComplete&&(CarInstalled&31)==31,TEXT("car_battery_and_test_drive_complete"));
 TestSaving=true;Save();CarInstalled=0;CarComplete=false;Load();TestSaving=false;
 Check(CarComplete&&(CarInstalled&31)==31,TEXT("car_progress_survives_reload"));
 CarKey(EKeys::R);Check(!CarComplete&&CarInstalled==0,TEXT("car_replay_resets_only_car_progress"));
 CarClick({100,780});Check(!CarMode&&Stage==0&&MenuPage==1,TEXT("car_returns_to_level_selection"));
 Stage=0;FirstClear=false;MenuPage=1;RequestLevel(2);
 Check(!CarMode&&MenuPage==1&&PendingLevel==0,TEXT("level_two_locked_until_level_one_clear"));
 FirstClear=true;RequestLevel(2);Check(Stage==0&&MenuPage==2,TEXT("level_selection_shows_loading_screen"));FinishLevelLoad();
 Check(CarMode&&CarReturnStage==0&&CarInstalled==0&&!CarReleased,TEXT("unlocked_level_two_starts_fresh"));
 CarClick(Screen(FVector(350,0,360)));Up({800,100});
 Check(CarInstalled==0&&!CarPartDrag,TEXT("car_wrong_drop_returns_tyre_to_rack"));
 CarClick(Screen(FVector(350,0,360)));Up(Screen(FVector(-127,-12,198)));
 Check((CarInstalled&1)!=0&&!CarPartDrag,TEXT("car_pointer_drag_drop_fits_tyre"));
 for(int I=1;I<4;I++)FitCarPart(I);
 FitCarPart(5);CarEngineClick({785,390});Up({935,390});
 Check(CarEnginePuzzle&&!CarEngineAligned&&CarCylinders==0,TEXT("engine_requires_alignment_before_cylinders"));
 CarEngineClick({550,490});CarEngineClick({550,490});CarEngineClick({600,560});CarEngineClick({785,390});Up({600,620});
 Check(CarEngineAligned&&CarCylinders==0,TEXT("engine_outside_drop_keeps_unsolved_order"));
 CarEngineClick({785,390});CarUp({1085,390});CarEngineClick({785,390});CarUp({935,390});
 Check(CarCylinders==3&&!CarEnginePuzzle,TEXT("engine_accepts_three_cylinders_in_order"));
 FitCarPart(4);CarClick({1250,780});
 Check(CarWiring&&!CarReleased,TEXT("car_lever_waits_for_power_and_headlights"));
 for(int I=0;I<4;I++){WireStart=I<2?0:1;PenPoints.Empty();FinishTrack(I+2);}TestCircuit();StageTime+=2.5f;SendCircuit();FitCarPart(6);CarKey(EKeys::L);TickCar(2.1f);CarKey(EKeys::Right);
 Check(CarHead&&CarReleased&&CarTravelTarget>0,TEXT("car_headlights_release_and_forward_control"));
 Check(CarLiftDeck&&CarLiftPistons.Num()==2&&CarBattery->GetStaticMesh()->GetName()==TEXT("HeroBattery"),TEXT("car_has_hydraulic_lift_and_detailed_battery"));
 CarReleased=false;CarTestTime=-1;CarReleaseProgress=0;CarTravel=CarTravelTarget=0;UpdateCarLevel();
 CarClick({1250,780});Check(!CarReleased,TEXT("removed_release_button_has_no_hit_target"));
 FVector2D Knob=Screen(LeverKnob->GetComponentLocation());CarClick(Knob);Up(Knob+FVector2D(0,30));
 Check(!CarReleased,TEXT("car_short_physical_lever_pull_does_not_release"));
 CarClick(Knob);Up(Knob+FVector2D(0,100));
 Check(CarReleased&&CarComplete,TEXT("car_full_physical_lever_pull_starts_lift"));
 const bool SavedReduced=Reduced;Reduced=false;TickCar(1.f);Check(CarReleaseProgress>.45f&&CarReleaseProgress<.55f,TEXT("hydraulic_lowering_has_midpoint"));
 StageTime=CarTestTime+2.7f;TickCar(1.f);UpdateCarLevel();
 Check(CarReleaseProgress==1.f&&CarWheelieAngle>20&&CarCelebrationCue>=1,TEXT("car_celebration_wheelie_starts_after_lowering"));
 StageTime=CarTestTime+10.1f;TickCar(.1f);Check(CarTestTime<0&&CarWheelieAngle==0,TEXT("car_celebration_finishes_and_returns_to_driving"));Reduced=SavedReduced;
 CarClick({100,365});Check(CarTravelTarget>0,TEXT("stacked_left_forward_button_drives"));
 const int PreviousPaint=CarPaintIndex;CarKey(EKeys::P);CarKey(EKeys::B);
 Check(CarPaintIndex!=PreviousPaint&&!CarLampOn,TEXT("car_paint_and_headlight_controls_work"));
 CarClick({100,780});
 Check(!CarMode&&Stage==0&&FirstClear,TEXT("return_to_menu_preserves_level_unlocks"));
 CarInstalled=31;CarReleased=true;CarHead=true;CarEverComplete=true;const int SavedCarPaint=CarPaintIndex;
 RequestLevel(2);FinishLevelLoad();Check(CarMode&&CarInstalled==0&&!CarHead&&!CarReleased&&CarPaintIndex==SavedCarPaint&&CarEverComplete,TEXT("replayed_car_level_resets_repair_preserves_style_and_completion"));
 Settings=true;Paused=true;Click({700,700});Check(Stage==0&&!CarMode&&!Settings&&!Paused&&MenuPage==0,TEXT("settings_main_menu_clears_gameplay_overlay"));
 MenuClick({800,720});Check(MenuPage==1,TEXT("main_menu_play_opens_level_selector"));RequestLevel(1);FinishLevelLoad();
 bool EmptyParts=true;for(auto* P:Parts)EmptyParts&=!P->Installed;
 Check(Stage==1&&!CarMode&&EmptyParts&&FirstClear,TEXT("selected_level_one_starts_at_first_repair"));
 TestSaving=true;Save();FirstClear=false;CarEverComplete=false;Load();TestSaving=false;
 Check(FirstClear&&CarEverComplete,TEXT("level_unlocks_and_completion_survive_save_reload"));
 FString JSON=TEXT("{\n  \"suite\":\"Robot Rescue Level 1 runtime\",\n  \"passed\":")+(All?FString(TEXT("true")):FString(TEXT("false")))+TEXT(",\n  \"checks\":[\n")+FString::Join(Checks,TEXT(",\n"))+TEXT("\n  ]\n}\n");
 FString TestPath=FPaths::ProjectSavedDir()/TEXT("RobotRescueTestResults.json");FFileHelper::SaveStringToFile(JSON,*TestPath);UE_LOG(LogTemp,Display,TEXT("RR_TEST_RESULT %s %s"),All?TEXT("PASS"):TEXT("FAIL"),*TestPath);
}
