#include "RescueGame.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

namespace {
const FLinearColor Dark(.012f,.025f,.035f),Chrome(.58f,.64f,.68f),Amber(1,.48f,.06f),Ink(.01f,.03f,.04f,.96f),TextColor(.91f,.95f,1),Glow(.04f,.85f,1);
const FVector Sockets[]={{-127,-12,198},{-72,105,198},{127,12,198},{72,-105,198},{0,-62,250},{0,0,265},{0,-60,300}};
const FVector Racks[]={{350,0,360},{350,0,275},{-350,0,360},{-350,0,275},{350,0,150},{-350,0,455},{-350,0,455}};
bool Hit(FVector2D P,float X,float Y,float W,float H){return P.X>=X&&P.X<=X+W&&P.Y>=Y&&P.Y<=Y+H;}
}

void ARescueWorld::BuildCarLevel(){
 auto Model=[&](const TCHAR* Name,FVector P){
  auto* Mesh=NewObject<UStaticMeshComponent>(this);
  Mesh->SetupAttachment(RootComponent);
  Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/%s.%s"),Name,Name)));
  Mesh->SetRelativeLocation(P);Mesh->SetRelativeRotation(FRotator(0,180,0));
  Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetVisibility(false);Mesh->RegisterComponent();return Mesh;
 };
 auto* Body=Model(TEXT("ZipRuggedBody"),FVector(0,0,270));
 Body->SetRelativeRotation(FRotator(0,155,0));
 CarBody.Add(Body);CarBodyRest.Add(FVector(0,0,270));
 for(int I=0;I<Body->GetNumMaterials();I++){
  auto* Base=Body->GetMaterial(I);if(!Base)continue;
  if(Base->GetName().Contains(TEXT("HeroPaint")))CarPaintMaterials.Add(Body->CreateDynamicMaterialInstance(I));
  if(Base->GetName().Contains(TEXT("HeroGlow")))CarLampMaterials.Add(Body->CreateDynamicMaterialInstance(I));
 }
 for(int I=0;I<4;I++)CarWheels.Add(Model(TEXT("ZipRuggedTire"),Racks[I]));
 CarBattery=Model(TEXT("HeroBattery"),Racks[4]);CarBattery->SetVisibility(false);
 CarLiftDeck=Box(TEXT("Cube"),FVector(0,0,150),FVector(310,260,16),Chrome);
 CarLiftDeck->SetRelativeRotation(FRotator(0,-25,0));
 for(int Side:{-1,1}){
  auto* Piston=Box(TEXT("Cylinder"),FVector(Side*65,0,72),FVector(28,28,144),Chrome);CarLiftPistons.Add(Piston);
  auto* Pad=Box(TEXT("Cube"),FVector(Side*55,0,176),FVector(35,100,36),Dark);CarLiftPads.Add(Pad);
 }
 for(int I=0;I<4;I++){
  auto* Puff=Box(TEXT("Plane"),FVector::ZeroVector,FVector(40,40,40),Chrome,FRotator(0,0,90));
  auto* Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/M_ZipSmoke.M_ZipSmoke")),this);
  Puff->SetMaterial(0,Mat);Puff->SetCastShadow(false);Puff->SetVisibility(false);CarSmoke.Add(Puff);CarSmokeMaterials.Add(Mat);
 }
 CarEngine=Model(TEXT("ZipEngine"),Racks[5]);
 CarFacePart=Box(TEXT("Cube"),Racks[6],FVector(100,22,50),Dark);CarFacePart->SetVisibility(false);
 for(int Side:{-1,1}){
  auto* Light=NewObject<UPointLightComponent>(this);Light->SetupAttachment(RootComponent);
  Light->SetIntensity(1200);Light->SetAttenuationRadius(650);Light->SetSourceRadius(25);
  Light->SetCastShadows(false);Light->SetLightColor(Glow);Light->SetVisibility(false);Light->RegisterComponent();CarHeadlights.Add(Light);
 }
 BuildDesertBay();
 BuildZipBeacon();
}

void ARescueWorld::OpenCarLevel(){
 if(!FirstClear){Say(TEXT("Complete Level 1 to unlock Zip's rescue."));return;}
 CarInstalled=0;CarComplete=false;CarEnginePlaced=false;CarEngineAligned=false;CarEnginePuzzle=false;CarCylinders=0;CarWired=false;CarWiring=false;CarHead=false;CarReleased=false;CarSelected=-1;CarPartDrag=false;CarCylinderDrag=-1;CarLampOn=true;CarWheelieAngle=0;CarTravel=CarTravelTarget=CarReleaseProgress=0;
 for(int I=0;I<3;I++)CarOrder[I]=I==0?3:I;

 if(!CarMode)CarReturnStage=Stage;
 CarMode=true;Stage=10;WorkshopJob=0;Collection=false;CarSelected=-1;CarTestTime=-1;
 Lever=CarReleased?1.f:0.f;CarReleaseProgress=CarReleased?1.f:0.f;CarWheelieAngle=0;CarCelebrationCue=0;
 if(CarEngineAligned)CarEngineOffset=0;
 CarEnginePuzzle=CarEnginePlaced&&CarCylinders<3;
 CarWiring=(CarInstalled&16)&&!CarWired&&!CarEnginePuzzle;
 for(auto* Mesh:BodyMeshes)Mesh->SetVisibility(false);
 for(auto* Part:Parts)Part->SetActorHiddenInGame(true);
 for(auto* Chain:Chains)Chain->SetVisibility(false);
 UpdateCarLevel();Say(TEXT("Hello, engineer! I am Zip. Can you help me roll again?"));Save();
}

void ARescueWorld::UpdateCarLevel(){
 UpdateDesertBay();
 UpdateZipBeacon();
 // Zip faces the camera, so forward motion travels toward the front of the room.
 const FVector Shift(CarTravel*.35f,-CarTravel,-158.f*CarReleaseProgress);
 const FQuat Tilt(FVector(.9063f,-.4226f,0),FMath::DegreesToRadians(-CarWheelieAngle));
 const FVector Pivot(27.5f,58.5f,198);
 auto Pose=[&](FVector P){return Pivot+Tilt.RotateVector(P-Pivot)+Shift;};
 const bool Celebrating=CarReleased&&CarTestTime>=0&&StageTime-CarTestTime<ZipCelebrationSeconds;
 const bool Lamps=CarLampOn&&(Reduced||!Celebrating||FMath::Fmod(StageTime-CarTestTime,.8f)<.55f);
 if(CarLiftDeck){CarLiftDeck->SetVisibility(CarMode);CarLiftDeck->SetRelativeLocation(FVector(0,0,150-158*CarReleaseProgress));}
 for(int I=0;I<CarLiftPistons.Num();I++){
  const float H=FMath::Max(.01f,144.f*(1-CarReleaseProgress));
  CarLiftPistons[I]->SetVisibility(CarMode&&CarReleaseProgress<.995f);CarLiftPistons[I]->SetRelativeScale3D(FVector(.28f,.28f,H/100));CarLiftPistons[I]->SetRelativeLocation(FVector(I?65:-65,0,H/2));
  CarLiftPads[I]->SetVisibility(CarMode&&CarReleaseProgress<.995f);CarLiftPads[I]->SetRelativeScale3D(FVector(.35f,1.f,FMath::Max(.001f,.36f*(1-CarReleaseProgress))));CarLiftPads[I]->SetRelativeLocation(FVector(I?55:-55,0,158-158*CarReleaseProgress+18*(1-CarReleaseProgress)));
 }
 const float DanceTime=StageTime-CarTestTime-ZipLiftSeconds;
 // A short cluster of expanding sprites, followed by a gap, reads as a tailpipe puff.
 for(int I=0;I<CarSmoke.Num();I++){
  const float Age=FMath::Fmod(FMath::Max(0.f,DanceTime),1.f)-I*.045f;
  const bool Show=CarMode&&Celebrating&&!Reduced&&CarReleaseProgress>=1.f&&DanceTime>=0.f&&Age>=0.f&&Age<.65f;
  CarSmoke[I]->SetVisibility(Show);
  if(!Show)continue;
  const float Growth=Age/.65f;
  const FVector Drift(-Age*85,Age*58,Age*66+I*3.f);
  CarSmoke[I]->SetRelativeLocation(Pose(FVector(-105,95,190))+Drift);
  CarSmoke[I]->SetRelativeScale3D(FVector(.22f+Growth*.54f));
  CarSmokeMaterials[I]->SetScalarParameterValue(TEXT("Opacity"),.37f*FMath::Sin(PI*Growth));
 }
 for(int I=0;I<CarBody.Num();I++){
  auto* Mesh=CarBody[I];if(Mesh->IsVisible()!=CarMode)Mesh->SetVisibility(CarMode);
  if(CarMode){Mesh->SetRelativeLocation(Pose(CarBodyRest[I]));Mesh->SetRelativeRotation((Tilt*FRotator(0,155,0).Quaternion()).Rotator());}
 }
 for(int I=0;I<CarWheels.Num();I++){
  auto* Wheel=CarWheels[I];if(Wheel->IsVisible()!=CarMode)Wheel->SetVisibility(CarMode);
  if(CarMode){Wheel->SetRelativeLocation(CarPartDrag&&CarSelected==I?World(Mouse):(CarInstalled&(1<<I))?Pose(Sockets[I]):Racks[I]);Wheel->SetRelativeRotation((CarInstalled&(1<<I))?(Tilt*FRotator(0,155,CarTravel*2).Quaternion()).Rotator():FRotator(0,90,0));}
 }
 for(auto* Tire:CarTower)if(Tire->IsVisible()!=CarMode)Tire->SetVisibility(CarMode);
 if(CarBattery){
  // Once fitted, the battery sits inside the chassis rather than over the face.
  CarBattery->SetVisibility(CarMode&&!(CarInstalled&16));
  if(CarMode&&!(CarInstalled&16))CarBattery->SetRelativeLocation(CarPartDrag&&CarSelected==4?World(Mouse):Racks[4]);
 }
 if(CarEngine){CarEngine->SetVisibility(CarMode&&!CarEnginePuzzle&&CarCylinders<3);if(CarMode)CarEngine->SetRelativeLocation(CarPartDrag&&CarSelected==5?World(Mouse):CarEnginePlaced?Sockets[5]+Shift:Racks[5]);}
 if(CarFacePart){CarFacePart->SetVisibility(CarMode&&CarWired&&!CarHead);if(CarMode)CarFacePart->SetRelativeLocation(CarPartDrag&&CarSelected==6?World(Mouse):Racks[6]);}
 for(int I=0;I<CarHeadlights.Num();I++){
  const bool Visible=CarMode&&CarHead&&Lamps;
  if(CarHeadlights[I]->IsVisible()!=Visible)CarHeadlights[I]->SetVisibility(Visible);
  if(Visible){CarHeadlights[I]->SetRelativeLocation(Pose(FVector(I?45.f:-45.f,-110,295)));CarHeadlights[I]->SetLightColor(CarLook?FLinearColor(1.f,.55f,.09f):Glow);}
 }
 for(auto* M:CarLampMaterials)M->SetVectorParameterValue(TEXT("Color"),CarHead&&Lamps?(CarLook?FLinearColor(1.f,.55f,.09f):Glow):FLinearColor::Black);
 const FLinearColor Paint[]={FLinearColor(.95f,.52f,.035f),FLinearColor(.13f,.56f,.88f),FLinearColor(.96f,.38f,.10f),FLinearColor(.36f,.76f,.39f)};
 for(auto* M:CarPaintMaterials){M->SetVectorParameterValue(TEXT("Color"),Paint[CarPaintIndex%4]);M->SetScalarParameterValue(TEXT("Roughness"),.43f);M->SetScalarParameterValue(TEXT("Metallic"),.05f);}
}

void ARescueWorld::FitCarPart(int32 Part){
 if(!CarMode)return;
 if(Part>=0&&Part<4){
  if(CarInstalled&(1<<Part))return;
  CarInstalled|=1<<Part;PlayEffect(TEXT("ZipAirWrench"));Say(TEXT("Wheel secured!"));
 }else if(Part==5){
  if((CarInstalled&15)!=15){Say(TEXT("Let's fit the four wheels first."));return;}
  CarEnginePlaced=true;CarEnginePuzzle=true;CarEngineAligned=false;CarEngineOffset=2;CarOrder[0]=3;CarOrder[1]=1;CarOrder[2]=2;CarCylinderDrag=-1;
  PlayEffect(TEXT("ZipEngineFit"));
  Say(TEXT("Align the engine, then place cylinders one, two and three."));
 }else if(Part==4){
  if(CarCylinders<3){Say(TEXT("Finish the engine before fitting the battery."));return;}
  CarInstalled|=16;CarWiring=true;WireStart=-1;CircuitPassed=false;TestStarted=-100;
  for(int C=0;C<4;C++){Tracks[C].Empty();PreviousTracks[C].Empty();TrackStart[C]=TrackEnd[C]=-1;}
  Say(TEXT("Let's wire the battery to my engine."));
 }else if(Part==6){
  if(!CarWired)return;
  CarHead=true;CarLampOn=true;PlayEffect(TEXT("HeadAttach"));AwakenTime=0;
  Say(TEXT("My headlights are shining! Pull the lever when you're ready."));
 }
 CarSelected=-1;UpdateCarLevel();Save();
}

void ARescueWorld::CarEngineClick(FVector2D P){
 if(Hit(P,1110,180,220,48)){CarEnginePuzzle=false;return;}
 if(Hit(P,510,475,180,50)){if(CarEngineAligned)return;CarEngineOffset=FMath::Max(-3,CarEngineOffset-1);return;}
 if(Hit(P,910,475,180,50)){if(CarEngineAligned)return;CarEngineOffset=FMath::Min(3,CarEngineOffset+1);return;}
 if(Hit(P,580,545,440,50)){
  if(CarEngineAligned)return;
  if(CarEngineOffset!=0){Say(TEXT("Move the engine until the alignment marks meet."));return;}
  CarEngineAligned=true;PlayEffect(TEXT("ZipEngineFit"));Save();Say(TEXT("Engine aligned. Swap the cylinders into one, two, three order."));return;
 }
 for(int I=0;I<3;I++)if(Hit(P,730+I*150,345,110,100)){
  if(!CarEngineAligned){Say(TEXT("Align the engine first."));return;}
  CarCylinderDrag=I;return;
 }
}

void ARescueWorld::CarUp(FVector2D P){
 if(CarEnginePuzzle&&CarCylinderDrag>=0){
  int From=CarCylinderDrag;CarCylinderDrag=-1;
  for(int I=0;I<3;I++)if(Hit(P,730+I*150,345,110,100)){
   Swap(CarOrder[From],CarOrder[I]);PlayEffect(TEXT("GearAttach"));
   if(CarOrder[0]==1&&CarOrder[1]==2&&CarOrder[2]==3){CarCylinders=3;CarEnginePuzzle=false;PlayEffect(TEXT("MotorEffect"));Say(TEXT("All three cylinders in order! Fit the battery."));Save();}
   else Say(TEXT("Swap the cylinders to match one, two, three."));
   return;
  }
  return;
 }
 if(CarPartDrag){
  CarPartDrag=false;
  if(CarSelected>=0&&FVector2D::Distance(P,Screen(Sockets[CarSelected]))<70)FitCarPart(CarSelected);
  else {CarSelected=-1;Say(TEXT("Try again. Drop the part inside its glowing socket."));}
  UpdateCarLevel();
 }
}

void ARescueHUD::DrawCarEngine(){
 Rect(350,135,900,600,Ink);Rect(350,135,900,5,Glow);Text(TEXT("ENGINE ASSEMBLY"),390,165,27,TextColor);Button(TEXT("CLOSE"),1110,180,120);
 Text(TEXT("SCHEMATIC: 1 - 2 - 3"),390,260,18,Chrome);
 Stroke({420,390},{650,390},Glow,4);
 for(int I=0;I<3;I++){float X=435+I*95;Ring({X,390},27,Glow,3);Text(FString::FromInt(I+1),X-8,375,23,TextColor);}
 Text(TEXT("Match this cylinder order"),390,435,16,TextColor);
 Text(W->CarEngineAligned?TEXT("DRAG CYLINDERS TO SWAP"):TEXT("MATCH THE HORIZONTAL LINES"),730,270,18,TextColor);
 Stroke({730,390},{1140,390},Glow,4);
 const float Y=390+W->CarEngineOffset*18;
 Stroke({730,Y},{1140,Y},Amber,3);
 for(int I=0;I<3;I++){
  FVector2D P(785+I*150,Y);
  if(W->CarCylinderDrag==I)P=W->Mouse;
  Rect(P.X-42,P.Y-40,84,80,Chrome);Rect(P.X-35,P.Y-33,70,66,Dark);
  Text(FString::FromInt(W->CarOrder[I]),P.X-9,P.Y-18,28,Amber);
 }
 Button(TEXT("MOVE UP"),510,475,180);Button(TEXT("MOVE DOWN"),910,475,180);
 Button(W->CarEngineAligned?TEXT("ENGINE ALIGNED"):TEXT("ALIGN ENGINE [A]"),580,545,440,!W->CarEngineAligned);
 Text(TEXT("Drag a cylinder onto another to swap their positions."),480,640,18,TextColor);
}

void ARescueWorld::CarClick(FVector2D P){
 if(CarEnginePuzzle){CarEngineClick(P);return;}
 if(CarWiring){
  if(Hit(P,970,160,186,48)){ChooseChallenge(false);MonitorMessage=TEXT("Follow the glowing pads.");Say(TEXT("Connect red to plus and black to minus."),TEXT("Wiring"));return;}
  if(Hit(P,1160,160,202,48)){ChooseChallenge(true);MonitorMessage=TEXT("Route around the striped guard.");return;}
  if(Hit(P,240,690,190,48)||Hit(P,445,690,190,48)){WireColor=P.X<440?0:1;WireStart=-1;PenDown=false;return;}
  if(Hit(P,650,690,130,48)){Eraser=!Eraser;WireStart=-1;return;}
  if(Hit(P,795,690,130,48)){UndoTrack();return;}
  if(Eraser&&Hit(P,710,275,650,330)){PreviousTracks[ActiveTrack]=Tracks[ActiveTrack];PreviousStart[ActiveTrack]=TrackStart[ActiveTrack];PreviousEnd[ActiveTrack]=TrackEnd[ActiveTrack];Tracks[ActiveTrack].Empty();TrackStart[ActiveTrack]=TrackEnd[ActiveTrack]=-1;CircuitPassed=false;TestStarted=-100;return;}
  if(Hit(P,940,690,185,48)){TestCircuit();return;}
  if(Hit(P,1140,690,220,48)){SendCircuit();return;}
  int T=HitTerminal(P);
  if(T>=0){if(WireStart>=0&&T>=2){FinishTrack(T);return;}if(T<2){WireColor=T;WireStart=T;ActiveTrack=T*2;PenPoints={Terminal(T)};PenDown=true;}return;}
  if(Challenge&&WireStart>=0&&Hit(P,735,310,610,290))AddRouteWaypoint(P);
  return;
 }
 if(CarReleased&&Hit(P,390,760,300,48)){
  if(Cheer)Cheer->Stop();
  CarInstalled=0;CarComplete=false;CarEnginePlaced=false;CarEngineAligned=false;CarEnginePuzzle=false;CarCylinders=0;CarWired=false;CarHead=false;CarReleased=false;CarSelected=-1;Lever=0;CarTestTime=-1;CarWheelieAngle=0;CarTravel=CarTravelTarget=CarReleaseProgress=0;
  UpdateCarLevel();Save();Say(TEXT("Let's build Zip again!"));return;
 }
 if(Hit(P,720,760,180,48)){CarPaintIndex=(CarPaintIndex+1)%4;Save();return;}
 if(Hit(P,920,760,180,48)){CarLook=(CarLook+1)%2;Save();return;}
 if(CarEnginePlaced&&CarCylinders<3&&Hit(P,1200,760,300,48)){CarEnginePuzzle=true;return;}
 if((CarInstalled&16)&&!CarWired&&Hit(P,1200,760,300,48)){CarWiring=true;return;}
 if(CarReleased){
  if(CarReleaseProgress<.99f)return;
  if(Hit(P,75,280,250,48)){CarTestTime=-1;CarWheelieAngle=0;CarTravelTarget=FMath::Clamp(CarTravelTarget-100.f,-240.f,240.f);}
  if(Hit(P,75,345,250,48)){CarTestTime=-1;CarWheelieAngle=0;CarTravelTarget=FMath::Clamp(CarTravelTarget+100.f,-240.f,240.f);}
  if(Hit(P,75,410,250,48)){PlayEffect(TEXT("ZipHorn"));Say(TEXT("Beep beep!"));}
  if(Hit(P,75,475,250,48))CarLampOn=!CarLampOn;
  return;
 }
 if(CarHead&&LeverKnob&&FVector2D::Distance(P,Screen(LeverKnob->GetComponentLocation()))<60){LeverDrag=true;LeverStartY=P.Y;Lever=0;return;}
 for(int I=0;I<7;I++){
  bool Available=I<4?!(CarInstalled&(1<<I)):I==4?CarCylinders==3&&!(CarInstalled&16):I==5?(CarInstalled&15)==15&&!CarEnginePlaced:CarWired&&!CarHead;
  if(Available&&FVector2D::Distance(P,Screen(Racks[I]))<65){CarSelected=I;CarPartDrag=true;Say(TEXT("Drag the part to its glowing socket."));return;}
 }
 if(CarSelected>=0){
  if(FVector2D::Distance(P,Screen(Sockets[CarSelected]))<85)FitCarPart(CarSelected);
  else Say(TEXT("Almost! Look for the glowing matching socket."));
 }
}

void ARescueWorld::CarKey(FKey K){
 if(CarEnginePuzzle){
  if(K==EKeys::Left)CarEngineClick({550,490});
  if(K==EKeys::Right)CarEngineClick({950,490});
  if(K==EKeys::A)CarEngineClick({600,560});
  if(K==EKeys::One||K==EKeys::Two||K==EKeys::Three){int I=K==EKeys::One?0:K==EKeys::Two?1:2;FVector2D P(785+150*I,390);if(CarCylinderDrag<0)CarEngineClick(P);else CarUp(P);}
  return;
 }
 if(CarWiring){
  if(K==EKeys::One||K==EKeys::Two){WireColor=K==EKeys::One?0:1;WireStart=-1;}
  if(K==EKeys::Tab)Focus=(Focus+1)%6;
  if(K==EKeys::T)TestCircuit();if(K==EKeys::S)SendCircuit();if(K==EKeys::Z)UndoTrack();
  if(K==EKeys::W&&Challenge&&WireStart>=0){AddRouteWaypoint({919,315});AddRouteWaypoint({1068,315});}
  if(K==EKeys::Enter||K==EKeys::SpaceBar){if(WireStart<0&&Focus<2){WireColor=Focus;WireStart=Focus;PenPoints={Terminal(Focus)};}else if(WireStart>=0&&Focus>=2)FinishTrack(Focus);}
  return;
 }
 if(K==EKeys::BackSpace){ReturnToMenu();MenuPage=1;return;}
 if(K==EKeys::R){CarClick({420,780});return;}
 if(K==EKeys::P){CarClick({750,780});return;}
 if(K==EKeys::F){CarClick({950,780});return;}
 if(CarReleased){
  if(K==EKeys::Left)CarClick({100,300});if(K==EKeys::Right)CarClick({100,365});
  if(K==EKeys::H)CarClick({100,430});if(K==EKeys::B)CarClick({100,495});return;
 }
 if(K==EKeys::L||(CarHead&&(K==EKeys::Enter||K==EKeys::SpaceBar))){ReleaseCar();return;}
 if(K==EKeys::E&&CarEnginePlaced&&CarCylinders<3){CarEnginePuzzle=true;return;}
 if(K==EKeys::Tab||K==EKeys::Right){
  for(int J=1;J<=7;J++){
   int I=(CarSelected+J+7)%7;
   bool Available=I<4?!(CarInstalled&(1<<I)):I==4?CarCylinders==3&&!(CarInstalled&16):I==5?(CarInstalled&15)==15&&!CarEnginePlaced:CarWired&&!CarHead;
   if(Available){CarSelected=I;break;}
  }
 }
 if((K==EKeys::Enter||K==EKeys::SpaceBar)&&CarSelected>=0)FitCarPart(CarSelected);
}

void ARescueHUD::DrawCarLevel(){
 Rect(590,24,420,68,Ink);Rect(590,24,420,4,Amber);Rect(590,88,420,4,Amber);Text(TEXT("LEVEL 2  /  ZIP"),665,42,28,TextColor);
 FString Task=(W->CarInstalled&15)!=15?TEXT("Fit all four wheels."):!W->CarEnginePlaced?TEXT("Fit the engine."):W->CarCylinders<3?TEXT("Complete the engine monitor."):!(W->CarInstalled&16)?TEXT("Fit the battery."):!W->CarWired?TEXT("Wire battery to engine."):!W->CarHead?TEXT("Attach the headlight face."):!W->CarReleased?TEXT("Pull the lever to release Zip."):W->CarReleaseProgress<.99f?TEXT("Lowering the hydraulic lift..."):W->CarTestTime>=0?TEXT("Zip is celebrating! Watch the victory dance."):TEXT("Drive Zip around the workshop!");
 Rect(440,108,720,38,Ink);Text(Task,462,118,17,TextColor);
 Text(W->CarReleased?TEXT("Use the driving buttons or arrow keys."):TEXT("Drag parts to their sockets. Tab and Enter work too."),490,151,14,TextColor);
 for(int I=0;I<7;I++){
  bool Available=I<4?!(W->CarInstalled&(1<<I)):I==4?W->CarCylinders==3&&!(W->CarInstalled&16):I==5?(W->CarInstalled&15)==15&&!W->CarEnginePlaced:W->CarWired&&!W->CarHead;
  if(!Available)continue;auto A=W->Screen(Racks[I]),B=W->Screen(Sockets[I]);Ring(A,30,W->CarSelected==I?Glow:Chrome,3);Ring(B,29,W->CarSelected==I?Glow:Chrome,3);
  if(I==6){Ring(A+FVector2D(-14,-3),6,Glow,3);Ring(A+FVector2D(14,-3),6,Glow,3);}

 }
 if(W->CarReleased)Button(TEXT("REPAIR AGAIN [R]"),390,760,300);
 Button(TEXT("PAINT [P]"),720,760,180);Button(TEXT("LOOK [F]"),920,760,180);
 if(W->CarEnginePlaced&&W->CarCylinders<3)Button(TEXT("OPEN ENGINE [E]"),1200,760,300);
 if((W->CarInstalled&16)&&!W->CarWired)Button(TEXT("OPEN WIRING"),1200,760,300);

 if(W->CarReleased){Button(TEXT("BACK [LEFT]"),75,280,250);Button(TEXT("FORWARD [RIGHT]"),75,345,250);Button(TEXT("HORN [H]"),75,410,250);Button(TEXT("LIGHTS [B]"),75,475,250);}
 if(W->CarEnginePuzzle)DrawCarEngine();
}

bool ARescueWorld::IsZipDancing() const{
 const float Elapsed=StageTime-CarTestTime;
 return CarMode&&CarReleased&&CarTestTime>=0.f&&CarReleaseProgress>=1.f&&Elapsed>=ZipLiftSeconds&&Elapsed<ZipCelebrationSeconds;
}

void ARescueWorld::BuildZipBeacon(){
 ZipBeaconRig=NewObject<USceneComponent>(this);
 ZipBeaconRig->SetupAttachment(RootComponent);
 ZipBeaconRig->RegisterComponent();

 ZipBeaconRotor=NewObject<USceneComponent>(this);
 ZipBeaconRotor->SetupAttachment(ZipBeaconRig);
 ZipBeaconRotor->RegisterComponent();

 auto AddPiece=[&](USceneComponent* Parent,const TCHAR* Shape,FVector Position,FVector Size,FLinearColor Color,bool Emissive=false){
  auto* Piece=Box(Shape,Position,Size,Color,FRotator::ZeroRotator,Emissive);
  Piece->AttachToComponent(Parent,FAttachmentTransformRules::KeepRelativeTransform);
  Piece->SetCastShadow(false);
  return Piece;
 };
 AddPiece(ZipBeaconRig,TEXT("Cylinder"),FVector(0,0,-19),FVector(91,91,14),Dark);
 AddPiece(ZipBeaconRig,TEXT("Cylinder"),FVector(0,0,-12),FVector(84,84,5),Chrome);
 AddPiece(ZipBeaconRig,TEXT("Cylinder"),FVector(0,0,46),FVector(27,27,10),Chrome);
 AddPiece(ZipBeaconRotor,TEXT("Sphere"),FVector(0,0,9),FVector(15,15,25),FLinearColor(1.f,.19f,.07f),true);
 AddPiece(ZipBeaconRotor,TEXT("Sphere"),FVector(-8,0,9),FVector(14,46,36),Chrome);
 auto* Lens=AddPiece(ZipBeaconRig,TEXT("Sphere"),FVector(0,0,12),FVector(80,80,71),FLinearColor(.45f,.01f,.018f));
 if(auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/M_ZipBeaconLens.M_ZipBeaconLens"))){
  Lens->SetMaterial(0,Material);
 }
 ZipBeaconCable=Box(TEXT("Cylinder"),FVector::ZeroVector,FVector(3,3,100),Chrome);
 ZipBeaconCable->SetVisibility(false);
 ZipBeaconRig->SetVisibility(false,true);

 ZipBeaconBeam=NewObject<USpotLightComponent>(this);
 ZipBeaconBeam->SetupAttachment(RootComponent);
 ZipBeaconBeam->SetIntensityUnits(ELightUnits::Candelas);
 ZipBeaconBeam->SetAttenuationRadius(1000.f);
 ZipBeaconBeam->SetInnerConeAngle(9.f);
 ZipBeaconBeam->SetOuterConeAngle(19.f);
 ZipBeaconBeam->SetLightColor(FLinearColor(1.f,.027f,.045f));
 ZipBeaconBeam->SetCastShadows(false);
 ZipBeaconBeam->SetVisibility(false);
 ZipBeaconBeam->RegisterComponent();
}

void ARescueWorld::UpdateZipBeacon(){
 if(!ZipBeaconRig)return;
 const bool Dancing=IsZipDancing();
 if(ZipBeaconRig->IsVisible()!=Dancing){
  ZipBeaconRig->SetVisibility(Dancing,true);
  ZipBeaconCable->SetVisibility(Dancing);
  ZipBeaconBeam->SetVisibility(Dancing);
 }
 if(!Dancing)return;

 const float DanceTime=StageTime-CarTestTime-ZipLiftSeconds;
 const float Drop=Reduced?1.f:FMath::SmoothStep(0.f,1.f, FMath::Clamp(DanceTime/.85f,0.f,1.f));
 const FVector RigPosition(0,-20,FMath::Lerp(630.f,434.f,Drop));
 ZipBeaconRig->SetRelativeLocation(RigPosition);
 const float CableLength=FMath::Max(1.f,682.f-RigPosition.Z-50.f);
 ZipBeaconCable->SetRelativeLocation(RigPosition+FVector(0,0,50.f+CableLength*.5f));
 ZipBeaconCable->SetRelativeScale3D(FVector(.03f,.03f,CableLength/100.f));

 const float Angle=Reduced?-.8f:DanceTime*2.5f-.8f;
 const FVector Direction(FMath::Cos(Angle),FMath::Sin(Angle),0);
 ZipBeaconRotor->SetRelativeRotation(Direction.Rotation());
 const FVector Lamp=RigPosition+FVector(0,0,9.f)+Direction*27.f;
 const FVector Aim=Direction*420.f+FVector(0,0,-340.f);
 ZipBeaconBeam->SetRelativeLocation(Lamp);
 ZipBeaconBeam->SetRelativeRotation(Aim.Rotation());
 ZipBeaconBeam->SetIntensity(Reduced?3200.f:20000.f*Drop);
}

void ARescueWorld::ReleaseCar(){
 if(!CarMode||!CarHead||!CarWired||CarReleased)return;
 CarReleased=true;CarComplete=true;CarEverComplete=true;CarTestTime=StageTime;CarCelebrationCue=0;Lever=1;
 PlayEffect(TEXT("ComingAlive"));Say(TEXT("Going down! Then it's time for Zip's victory dance!"));Save();
}

void ARescueWorld::TickCar(float Dt){
 if(!CarMode||!CarReleased)return;
 const float T=StageTime-CarTestTime;
 CarReleaseProgress=Reduced?1.f:FMath::Clamp(CarReleaseProgress+Dt/ZipLiftSeconds,0.f,1.f);
 CarWheelieAngle=0;
 if(CarTestTime>=0&&T<ZipCelebrationSeconds){
  if(CarCelebrationCue==0&&CarReleaseProgress>=1.f&&T>=ZipLiftSeconds){
   Cheer->SetSound(LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/ZipEngineStart.ZipEngineStart")));
   Cheer->Play();
   ++CarCelebrationCue;
  }
  const float CueTimes[]={0.f,4.1f,7.f,ZipCelebrationSeconds-2.f};
  const TCHAR* Cues[]={nullptr,TEXT("ZipHorn"),TEXT("ZipHorn"),TEXT("Yay")};
  while(CarCelebrationCue>0&&CarCelebrationCue<UE_ARRAY_COUNT(CueTimes)&&T>=CueTimes[CarCelebrationCue]){
   PlayEffect(Cues[CarCelebrationCue++]);
  }
  if(!Reduced&&T>ZipLiftSeconds){
   CarTravelTarget=FMath::Sin((T-ZipLiftSeconds)*1.7f)*65.f;
   float Phase=FMath::Fmod(T-ZipLiftSeconds,3.f);
   CarWheelieAngle=Phase<1.4f?24.f*FMath::Square(FMath::Sin(PI*Phase/1.4f)):0.f;
  }
 }else if(CarTestTime>=0){CarTestTime=-1;CarTravelTarget=0;Say(TEXT("That was fun! Your turn to drive."));}
 CarTravel=Reduced?CarTravelTarget:FMath::FInterpTo(CarTravel,CarTravelTarget,Dt,2.f);
}
