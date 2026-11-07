#include "RescueGame.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace {
const TCHAR* RobotNames[]={TEXT("Bolt"),TEXT("Parcel"),TEXT("Sprout")};
const TCHAR* PaintNames[]={TEXT("Steel"),TEXT("Amber"),TEXT("Mint"),TEXT("Berry")};
const TCHAR* FaceNames[]={TEXT("Classic"),TEXT("Happy eyes"),TEXT("Star eyes")};
const TCHAR* AccessoryNames[]={TEXT("None"),TEXT("Top hat"),TEXT("Beacon")};
const TCHAR* MoveNames[]={TEXT("Hop"),TEXT("Lasso"),TEXT("Robot Spin")};
const FLinearColor PaintColors[]={{.63f,.68f,.70f},{.95f,.43f,.045f},{.14f,.68f,.34f},{.62f,.17f,.68f}};
const FLinearColor ClassicEyes(.015f,.70f,1.f), HappyEyes(.42f,1.f,.05f), StarEyes(1.f,.11f,.48f);
const FLinearColor Ink(.014f,.032f,.044f,.97f), Edge(.1f,.77f,1), Paper(.89f,.94f,.98f);
const FLinearColor Amber(1,.56f,.18f), Mint(.22f,1,.60f), Muted(.54f,.69f,.74f);
const FVector2D PumpPads[]={{1135,368},{1135,454},{1285,368},{1285,454}};
bool Inside(FVector2D P,float X,float Y,float W,float H){
 return P.X>=X&&P.X<=X+W&&P.Y>=Y&&P.Y<=Y+H;
}
}

int32 ARescueWorld::GuidedPad() const{
 for(int I=0;I<4;I++)
  if(TrackStart[I]!=(I<2?0:1)||TrackEnd[I]!=I+2||Tracks[I].Num()<2)return I+2;
 return -1;
}

void ARescueWorld::ChooseChallenge(bool Enabled){
 Challenge=Enabled;
 Help=!Enabled;
 CircuitPassed=false;
 CircuitBlocked=false;
 TestStarted=-100;
 if(Stage==5){
  MonitorMessage=Enabled?TEXT("Challenge on. Route around the striped chip guard."):TEXT("Help on. Follow the glowing pads. Tap or draw a wire.");
  if(!Enabled)GiveHint();
 }
 Save();
}

void ARescueWorld::GiveHint(){
 if(Stage==5){
  const int Pad=GuidedPad();
  if(Pad<0){MonitorMessage=TEXT("All four pads are connected. Press Test Circuit.");return;}
  const bool Positive=Pad<4;
  MonitorMessage=Positive?TEXT("Start on the red + pad. Follow the glow to an A pad."):TEXT("Start on the black - pad. Follow the glow to a B pad.");
  Say(MonitorMessage,Positive?TEXT("Wiring"):TEXT("BlackHint"));
  return;
 }
 if(Stage==4){Say(Task,TEXT("BatteryHint"));return;}
 if(Stage>=1&&Stage<=6){Say(Task,Stage==2?TEXT("Chest"):TEXT("Mismatch"));return;}
 Say(Task);
}

bool ARescueWorld::TrackHitsObstacle(FVector2D& At) const{
 if(!Challenge)return false;
 // Include half the visible track width so a wire cannot graze the guard.
 const FBox Bounds(FVector(938,329,-1),FVector(1048,384,1));
 for(const auto& Track:Tracks){
  for(int I=1;I<Track.Num();I++){
   const FVector A(Track[I-1],0), B(Track[I],0);
   FVector Hit,Normal;
   float Time;
   if(FMath::LineExtentBoxIntersection(Bounds,A,B,FVector::ZeroVector,Hit,Normal,Time)){
    At=FVector2D(Hit.X,Hit.Y);
    return true;
   }
  }
 }
 return false;
}

void ARescueWorld::AddRouteWaypoint(FVector2D P){
 if(WireStart<0)return;
 P.X=FMath::Clamp(P.X,735.,1345.);
 P.Y=FMath::Clamp(P.Y,310.,600.);
 if(PenPoints.IsEmpty()||FVector2D::Distance(PenPoints.Last(),P)>4)PenPoints.Add(P);
 PenDown=false;
}

void ARescueWorld::UpdateRepairReaction(float Dt){
 if(PowerCountdown>=0&&Stage==6){
  PowerCountdown+=Dt;
  const int Count=FMath::Max(0,3-FMath::FloorToInt(PowerCountdown/.65f));
  if(PowerCount!=Count){PowerCount=Count;Beep(Count?560.f:920.f);}
  if(!Count){PowerCountdown=-1;Say(TEXT("Power is flowing! Now attach my head."),TEXT("Power"));}
 }
 if(Stage<1||Stage>=8||ReactionPart<0)return;
 if(Reduced){
  for(int I=0;I<4;I++)if(Parts[I]->Installed)Parts[I]->SetActorRotation(FRotator::ZeroRotator);
  ReactionPart=-1;
  return;
 }
 const float Age=Clock-Reaction;
 const float Envelope=FMath::Clamp(Age/.16f,0.f,1.f)*FMath::Clamp((1.6f-Age)/.3f,0.f,1.f);
 if(ReactionPart<2){
  const float Side=ReactionPart?1.f:-1.f;
  Parts[ReactionPart]->SetActorRotation(FRotator(Side*(102.f+FMath::Sin(Age*14.f)*14.f)*Envelope,0,-25.f*Envelope));
 }else if(ReactionPart<4&&Parts[2]->Installed&&Parts[3]->Installed){
  const float Wobble=FMath::Sin(Age*17.f)*Envelope;
  Parts[2]->SetActorRotation(FRotator(Wobble*12.f,0,0));
  Parts[3]->SetActorRotation(FRotator(-Wobble*12.f,0,0));
  const float Lift=FMath::Max(0.f,FMath::Sin(Age*5.f))*Envelope*25.f;
  auto* Foot=Parts[ReactionPart];
  auto Tilt=Foot->GetActorRotation();
  Tilt.Roll=-Lift;
  Foot->SetActorRotation(Tilt);
  if(Age>=.64f&&Age-Dt<.64f)Beep(175);
 }
 if(Age>=1.6f)ReactionPart=-1;
}

void ARescueWorld::StartDanceReplay(){
 if(!FirstClear||Stage!=10||WorkshopJob)return;
 Collection=false;
 Stage=9;
 for(auto* Chain:Chains)Chain->SetVisibility(false);
 Refresh();
 Say(FString::Printf(TEXT("%s is ready to dance! Queue a move below."),RobotNames[ActiveRobot]));
}

void ARescueWorld::QueueDance(int32 Move){
 if(Stage!=9||Reduced||Move<0||Move>2||DanceQueue.Num()>=6)return;
 DanceQueue.Add(Move);
 Beep(650.f+Move*100.f);
}

void ARescueWorld::FireConfetti(){
 if(Stage==9&&!Reduced&&(ManualFoilTime<0||StageTime-ManualFoilTime>=2.6f)){
  ManualFoilTime=StageTime;
  Beep(880);
 }
}

void ARescueWorld::UpdateDanceControls(){
 if(DanceMove>=0&&StageTime>=DanceMoveUntil)DanceMove=-1;
 if(DanceMove<0&&DanceQueue.Num()&&!Reduced){
  DanceMove=DanceQueue[0];
  DanceQueue.RemoveAt(0);
  DanceMoveStart=StageTime;
  DanceMoveUntil=StageTime+1.75f;
 }
 if(Reduced){DanceQueue.Empty();DanceMove=-1;}
}

bool ARescueWorld::RobotUnlocked(int32 Robot) const{
 return Robot==0||(Robot>=1&&Robot<=2&&(JobMedals&(1<<(Robot-1)))!=0);
}

void ARescueWorld::BuildRobotStyle(){
 RobotStyles[1].Paint=1;
 RobotStyles[2].Paint=2;
 RobotStyles[2].Face=1;
 auto Collect=[&](UStaticMeshComponent* Mesh){
  for(int Slot=0;Slot<Mesh->GetNumMaterials();Slot++){
   auto* Base=Mesh->GetMaterial(Slot);
   if(!Base)continue;
   const bool Paint=Base->GetFName()==FName(TEXT("M_HeroPaint"));
   const bool EyesOnly=Base->GetFName()==FName(TEXT("M_HeroGlow"))&&Mesh->GetOwner()==Parts[8];
   if(!Paint&&!EyesOnly)continue;
   if(auto* Instance=Mesh->CreateDynamicMaterialInstance(Slot)){
    (Paint?StylePaint:StyleEyes).Add(Instance);
   }
  }
 };
 for(auto* Mesh:BodyMeshes)Collect(Mesh);
 for(auto* Part:Parts){
  TArray<UStaticMeshComponent*> Meshes;
  Part->GetComponents(Meshes);
  for(auto* Mesh:Meshes)Collect(Mesh);
 }
 auto HeadPiece=[&](const TCHAR* Shape,FVector Position,FVector Size,FLinearColor Color,bool Glow=false,float Pitch=0.f){
  auto* Mesh=Box(Shape,FVector::ZeroVector,Size,Color,FRotator(Pitch,0,0),Glow);
  Mesh->AttachToComponent(Parts[8]->GetRootComponent(),FAttachmentTransformRules::KeepRelativeTransform);
  Mesh->SetRelativeLocation(Position);
  Mesh->SetVisibility(false);
  return Mesh;
 };
 StyleBrim=HeadPiece(TEXT("Cylinder"),FVector(0,0,44),FVector(65,55,5),Ink);
 StyleCap=HeadPiece(TEXT("Cylinder"),FVector(0,0,60),FVector(43,40,28),FLinearColor(.095f,.075f,.14f));
 StyleBeacon=HeadPiece(TEXT("Sphere"),FVector(29,0,41),FVector(22,22,26),Amber,true);
 for(int X:{-22,22}){
  HappyFace.Add(HeadPiece(TEXT("Cube"),FVector(X-5,-45,7),FVector(13,1.5,3.5),HappyEyes,true,37));
  HappyFace.Add(HeadPiece(TEXT("Cube"),FVector(X+5,-45,7),FVector(13,1.5,3.5),HappyEyes,true,-37));
  for(float Angle:{0.f,45.f,90.f,135.f})
   StarFace.Add(HeadPiece(TEXT("Cube"),FVector(X,-45,8),FVector(16,1.5,3),StarEyes,true,Angle));
 }
 StyleSmile.Add(HeadPiece(TEXT("Cube"),FVector(-8,-45,-13),FVector(11,1.5,2.6),ClassicEyes,true,-27));
 StyleSmile.Add(HeadPiece(TEXT("Cube"),FVector(8,-45,-13),FVector(11,1.5,2.6),ClassicEyes,true,27));
 StyleSmile.Add(HeadPiece(TEXT("Cube"),FVector(0,-45,-15),FVector(10,1.5,2.6),ClassicEyes,true));
}

void ARescueWorld::ApplyRobotStyle(){
 const auto& Style=RobotStyles[WorkshopJob?WorkshopJob:ActiveRobot];
 for(auto* Paint:StylePaint)Paint->SetVectorParameterValue(TEXT("Color"),PaintColors[Style.Paint]);
 for(auto* Eye:StyleEyes)Eye->SetVectorParameterValue(TEXT("Color"),Style.Face==0?ClassicEyes:FLinearColor::Black);
 const bool HeadReady=Parts.IsValidIndex(8)&&Parts[8]->Installed;
 for(auto* Piece:HappyFace)Piece->SetVisibility(HeadReady&&Style.Face==1);
 for(auto* Piece:StarFace)Piece->SetVisibility(HeadReady&&Style.Face==2);
 for(auto* Piece:StyleSmile)Piece->SetVisibility(HeadReady&&Style.Face!=0);
 if(StyleBrim)StyleBrim->SetVisibility(HeadReady&&Style.Accessory==1);
 if(StyleCap)StyleCap->SetVisibility(HeadReady&&Style.Accessory==1);
 if(StyleBeacon)StyleBeacon->SetVisibility(HeadReady&&Style.Accessory==2);
}

void ARescueWorld::SelectRobot(int32 Robot){
 if(Robot<0||Robot>2||!RobotUnlocked(Robot))return;
 ActiveRobot=Robot;
 ApplyRobotStyle();
 Save();
}

void ARescueWorld::CycleStyle(int32 Feature){
 if(!FirstClear||Stage!=10||!Collection)return;
 auto& Style=RobotStyles[ActiveRobot];
 if(Feature==0)Style.Paint=(Style.Paint+1)%4;
 if(Feature==1)Style.Face=(Style.Face+1)%3;
 if(Feature==2)Style.Accessory=(Style.Accessory+1)%3;
 ApplyRobotStyle();
 Save();
 Beep(760);
}

void ARescueWorld::OpenWorkshopJob(int32 Job){
 if(Stage!=10||!FirstClear||Job<1||Job>2)return;
 WorkshopJob=Job;
 Collection=false;
 GearChoice=0;
 PumpStart=-1;
 PumpFocus=0;
 PumpSources[0]=PumpSources[1]=-1;
 JobTestTime=-1;
 JobPassed=false;
 JobMessage=Job==1?TEXT("Choose a gear, then test the lift."):TEXT("Tap a battery pad, then a pump pad.");
 ApplyRobotStyle();
}

void ARescueWorld::TapPumpTerminal(int32 TerminalId){
 if(WorkshopJob!=2||TerminalId<0||TerminalId>3)return;
 if(TerminalId<2){PumpStart=TerminalId;JobMessage=TEXT("Now tap a pump pad to connect it.");}
 else if(PumpStart>=0){
  PumpSources[TerminalId-2]=PumpStart;
  PumpStart=-1;
  JobPassed=false;
  JobTestTime=-1;
  JobMessage=TEXT("Wire fitted. Connect both pads and test.");
  Beep(700);
 }
}

void ARescueWorld::TestWorkshopJob(){
 if(!WorkshopJob)return;
 JobTestTime=StageTime;
 if(WorkshopJob==1){
  const int LiftForce=GearChoice+1; // 12, 24 or 36 teeth driven by a 12-tooth gear.
  JobPassed=LiftForce>=(Challenge?3:2);
  JobMessage=JobPassed?TEXT("Parcel lifted it! Delivery bot unlocked."):TEXT("Too heavy for that gear. Try a larger gear.");
 }else{
  JobPassed=PumpSources[0]==0&&PumpSources[1]==1;
  JobMessage=JobPassed?TEXT("Water is flowing! Garden bot unlocked."):TEXT("The pump stayed off. Match + to + and - to -.");
 }
 Beep(JobPassed?920:280);
 if(JobPassed){JobMedals|=1<<(WorkshopJob-1);Save();}
}

bool ARescueWorld::ClickWorkshop(FVector2D P){
 if(Stage==9){
  for(int Move=0;Move<3;Move++)if(Inside(P,87,380+Move*53,286,48)){QueueDance(Move);return true;}
  if(Inside(P,87,539,286,48)){FireConfetti();return true;}
  return false;
 }
 if(Stage!=10)return false;
 if(WorkshopJob){
  if(Inside(P,100,664,410,50)){WorkshopJob=0;ApplyRobotStyle();return true;}
  if(Inside(P,100,530,410,50)){TestWorkshopJob();return true;}
  if(JobPassed&&Inside(P,100,595,410,50)){
   const int Robot=WorkshopJob;
   WorkshopJob=0;
   SelectRobot(Robot);
   Collection=true;
   return true;
  }
  if(WorkshopJob==1){
   for(int I=0;I<3;I++)if(Inside(P,100+I*138,450,130,48)){GearChoice=I;JobPassed=false;JobTestTime=-1;JobMessage=TEXT("Gear ready. Test the lift.");}
  }else{
   for(int I=0;I<4;I++)if(FVector2D::Distance(P,PumpPads[I])<36)TapPumpTerminal(I);
  }
  return true;
 }
 if(Collection){
  for(int I=0;I<3;I++)if(Inside(P,100,260+I*63,410,48))SelectRobot(I);
  for(int I=0;I<3;I++)if(Inside(P,1095,340+I*65,392,48))CycleStyle(I);
  if(Inside(P,100,480,410,48))StartDanceReplay();
  if(Inside(P,100,548,410,48))Collection=false;
  return true;
 }
 if(Inside(P,100,300,410,48))Collection=true;
 if(Inside(P,100,363,410,48))OpenWorkshopJob(1);
 if(Inside(P,100,426,410,48))OpenWorkshopJob(2);
 if(Inside(P,100,489,410,48))StartDanceReplay();
 if(Inside(P,100,552,410,48))Reset();
 if(Inside(P,100,615,410,48))OpenCarLevel();
 return true;
}

bool ARescueWorld::KeyWorkshop(FKey K){
 if(Stage==9){
  if(K==EKeys::One||K==EKeys::Two||K==EKeys::Three){QueueDance(K==EKeys::One?0:K==EKeys::Two?1:2);return true;}
  if(K==EKeys::C){FireConfetti();return true;}
  return false;
 }
 if(Stage!=10)return false;
 if(WorkshopJob){
  if(K==EKeys::BackSpace){WorkshopJob=0;ApplyRobotStyle();return true;}
  if(K==EKeys::P&&JobPassed)return ClickWorkshop(FVector2D(150,610));
  if(K==EKeys::T){TestWorkshopJob();return true;}
  if(WorkshopJob==1&&(K==EKeys::One||K==EKeys::Two||K==EKeys::Three)){
   GearChoice=K==EKeys::One?0:K==EKeys::Two?1:2;
   JobPassed=false;
   JobTestTime=-1;
   JobMessage=TEXT("Gear ready. Test the lift.");
  }
  if(WorkshopJob==2){
   if(K==EKeys::Tab||K==EKeys::Right||K==EKeys::Left)PumpFocus=(PumpFocus+(K==EKeys::Left?3:1))%4;
   if(K==EKeys::Enter||K==EKeys::SpaceBar)TapPumpTerminal(PumpFocus);
  }
  return true;
 }
 if(K==EKeys::C){Collection=!Collection;return true;}
 if(K==EKeys::P){StartDanceReplay();return true;}
 if(Collection){
  if(K==EKeys::One||K==EKeys::Two||K==EKeys::Three)SelectRobot(K==EKeys::One?0:K==EKeys::Two?1:2);
  if(K==EKeys::BackSpace||K==EKeys::Enter)Collection=false;
  if(K==EKeys::V)CycleStyle(0);
  if(K==EKeys::B)CycleStyle(1);
  if(K==EKeys::N)CycleStyle(2);
 }else{
  if(K==EKeys::One)OpenWorkshopJob(1);
  if(K==EKeys::Two)OpenWorkshopJob(2);
  if(K==EKeys::L)OpenCarLevel();
  if(K==EKeys::Enter||K==EKeys::SpaceBar)Reset();
 }
 return true;
}

void ARescueHUD::DrawDanceControls(){
 if(W->Reduced)return;
 Rect(67,300,326,306,Ink);
 Rect(67,300,326,3,Edge);
 Text(TEXT("DANCE DECK"),87,320,22,Paper);
 FString Status=TEXT("Tap a move to queue it.");
 if(W->DanceMove>=0)Status=FString::Printf(TEXT("%s  /  %d queued"),MoveNames[W->DanceMove],W->DanceQueue.Num());
 Text(Status,87,351,14,Muted);
 Button(TEXT("HOP [1]"),87,380,286,W->DanceMove==0);
 Button(TEXT("LASSO [2]"),87,433,286,W->DanceMove==1);
 Button(TEXT("ROBOT SPIN [3]"),87,486,286,W->DanceMove==2);
 Button(TEXT("CONFETTI! [C]"),87,539,286);
}

void ARescueHUD::DrawWorkshop(){
 const bool Job=W->WorkshopJob!=0;
 Rect(76,143,458,Job?593:550,Ink);
 Rect(76,143,5,Job?593:550,Edge);
 Text(Job?(W->WorkshopJob==1?TEXT("PARCEL LIFT"):TEXT("WATERING ARM")):W->Collection?TEXT("ROBOT COLLECTION"):TEXT("WORKSHOP OPEN"),100,173,27,Paper);
 if(!Job&&!W->Collection){
  Text(TEXT("Rescue complete. Pick what to play next."),100,228,16,Muted);
  Text(FString::Printf(TEXT("%d / 3 robot friends  |  %d XP"),1+((W->JobMedals&1)?1:0)+((W->JobMedals&2)?1:0),W->XP),100,257,15,Amber);
  Button(TEXT("VISIT YOUR ROBOTS [C]"),100,300,410);
  Button(TEXT("DELIVERY JOB [1]"),100,363,410);
  Button(TEXT("GARDEN JOB [2]"),100,426,410);
  Button(TEXT("PLAY THE DISCO [P]"),100,489,410);
  Button(TEXT("REPLAY RESCUE [ENTER]"),100,552,410);
  Button(TEXT("LEVEL 2  /  ZIP [L]"),100,615,410);
  return;
 }
 if(W->Collection){
  Text(TEXT("Pick a robot to preview and customize."),100,223,16,Muted);
  for(int I=0;I<3;I++){
   const bool Open=W->RobotUnlocked(I);
   const FString State=Open?(W->ActiveRobot==I?TEXT("   /   SELECTED"):TEXT("")):TEXT("   /   FINISH JOB");
   Button(FString::Printf(TEXT("[%d] %s%s"),I+1,*FString(RobotNames[I]).ToUpper(),*State),100,260+I*63,410,Open);
  }
  Button(TEXT("PLAY [P]"),100,480,410);
  Button(TEXT("BACK TO WORKSHOP"),100,548,410);
  Rect(1070,228,442,329,Ink);
  Rect(1070,228,442,3,Amber);
  Text(FString::Printf(TEXT("STYLE   /   %s"),RobotNames[W->ActiveRobot]),1095,257,23,Paper);
  const auto& Style=W->RobotStyles[W->ActiveRobot];
  Button(FString::Printf(TEXT("PAINT  /  %s [V]"),PaintNames[Style.Paint]),1095,340,392);
  Button(FString::Printf(TEXT("FACE  /  %s [B]"),FaceNames[Style.Face]),1095,405,392);
  Button(FString::Printf(TEXT("ACCESSORY  /  %s [N]"),AccessoryNames[Style.Accessory]),1095,470,392);
  return;
 }
 Rect(1070,230,451,376,Ink);
 Rect(1070,230,451,3,Edge);
 Text(W->WorkshopJob==1?TEXT("LIVE LIFT TEST"):TEXT("PUMP CONTROL BOARD"),1098,254,21,Paper);
 const float Age=W->JobTestTime<0?-1.f:W->StageTime-W->JobTestTime;
 const float Progress=W->Reduced?1.f:FMath::Clamp(Age/1.1f,0.f,1.f);
 if(W->WorkshopJob==1){
  Text(TEXT("Help Parcel deliver a crate."),100,230,17,Paper);
  Text(W->Challenge?TEXT("Heavy load. Needs 3 units of lift."):TEXT("This load needs 2 units of lift."),100,265,16,Muted);
  Text(TEXT("Pick a driven gear for the 12-tooth motor."),100,300,15,Muted);
  for(int I=0;I<3;I++){
   const FString Label=FString::Printf(TEXT("%d TEETH"),12*(I+1));
   Button(Label,100+I*138,450,130,W->GearChoice==I);
  }
  const float DrivenRadius=21.f*(W->GearChoice+1);
  const float Turn=Age>=0&&!W->Reduced?FMath::Min(Age,1.2f)*6.0f:0;
  const FVector2D Small(1130,382),Large(1150+DrivenRadius,382);
  for(int G=0;G<2;G++){
   const float Radius=G?DrivenRadius:21.f;
   const FVector2D Center=G?Large:Small;
   const int Teeth=G?12*(W->GearChoice+1):12;
   Ring(Center,Radius-3,G?Amber:Edge,8);
   for(int I=0;I<Teeth;I++){
    const float A=I*2*PI/Teeth+(G?-Turn/(W->GearChoice+1):Turn);
    const FVector2D D(FMath::Cos(A),FMath::Sin(A));
    Stroke(Center+D*(Radius-4),Center+D*(Radius+4),G?Amber:Edge,3);
   }
   Ring(Center,5,Paper,3);
  }
  const float Rise=Age>=0?(W->JobPassed?95.f:10.f)*Progress:0.f;
  Stroke({1430,336},{1430,518-Rise},Muted,3);
  Rect(1391,518-Rise,78,47,Amber);
  Rect(1423,518-Rise,14,47,FLinearColor(.4f,.2f,.08f));
  Text(TEXT("CARGO"),1399,530-Rise,12,Ink);
  Text(FString::Printf(TEXT("Lift force: %d"),W->GearChoice+1),1110,542,17,Paper);
 }else{
  Text(TEXT("Help Sprout water the seedling."),100,230,17,Paper);
  Text(TEXT("Tap battery +, then pump +. Match both ends."),100,265,15,Muted);
  Text(TEXT("Repeat with -. Test to see if the pump runs."),100,296,15,Muted);
  if(!W->Challenge)Text(TEXT("Look for the glowing pad when you need help."),100,327,14,Edge);
  Text(TEXT("BATTERY"),1100,307,13,Muted);
  Text(TEXT("PUMP"),1256,307,13,Muted);
  for(int I=0;I<2;I++)if(W->PumpSources[I]>=0)
   Stroke(PumpPads[W->PumpSources[I]],PumpPads[I+2],W->PumpSources[I]==0?FLinearColor(.96f,.22f,.16f):Muted,5);
  for(int I=0;I<4;I++){
   const FVector2D P=PumpPads[I];
   Ring(P,22,Amber,4);
   Text(I%2?TEXT("-"):TEXT("+"),P.X-7,P.Y-13,20,Paper);
   const int Hint=W->PumpStart>=0?W->PumpStart+2:(W->PumpSources[0]!=0?0:W->PumpSources[1]!=1?1:-1);
   if(W->PumpFocus==I||W->PumpStart==I||(!W->Challenge&&Hint==I))Ring(P,29,Edge,2);
  }
  Stroke({1323,410},{1415,410},Muted,8);
  Stroke({1415,410},{1415,437},Muted,8);
  Rect(1386,548,64,32,Amber);
  const float Stem=W->JobPassed?70.f*Progress:20.f;
  Stroke({1418,550},{1418,550-Stem},Mint,5);
  if(W->JobPassed){
   Stroke({1418,533-Stem*.36f},{1398,518-Stem*.36f},Mint,4);
   Stroke({1418,528-Stem*.36f},{1437,512-Stem*.36f},Mint,4);
   Ring({1418,550-Stem},7,Amber,5);
   if(!W->Reduced)for(int I=0;I<6;I++){
    const float Y=440+FMath::Fmod(Age*110+I*17,97.f);
    Stroke({1410.f+(I%3)*6,Y},{1410.f+(I%3)*6,Y+7},Edge,3);
   }
  }
 }
 Text(W->JobMessage,100,390,13,W->JobPassed?Mint:Paper);
 Button(TEXT("TEST [T]"),100,530,410);
 if(W->JobPassed)Button(TEXT("MEET YOUR ROBOT [P]"),100,595,410);
 Button(TEXT("BACK [BACKSPACE]"),100,664,410);
}
