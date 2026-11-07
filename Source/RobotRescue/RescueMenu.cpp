#include "RescueGame.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"

void ARescueWorld::ReturnToMenu(){
 CarMode=false;CarWiring=false;CarEnginePuzzle=false;CarPartDrag=false;CarCylinderDrag=-1;
 CarTestTime=-1;CarWheelieAngle=0;Stage=0;MenuPage=0;MenuSelection=1;PendingLevel=0;MenuLoadTime=-1;
 Settings=false;Paused=false;NameEntry=false;WorkshopJob=0;Collection=false;
 Dragging=false;LeverDrag=false;PenDown=false;WireStart=-1;Selected=-1;AwakenTime=-1;
 ReactionPart=-1;PowerCountdown=-1;DanceQueue.Empty();
 if(Voice)Voice->Stop();if(Effects)Effects->Stop();if(Cheer)Cheer->Stop();
 for(auto* M:BodyMeshes)M->SetVisibility(true);
 UpdateCarLevel();Refresh();Save();
}

void ARescueWorld::RequestLevel(int32 Level){
 if(Level<1||Level>2||MenuPage==2)return;
 if(Level==2&&!FirstClear){Say(TEXT("Finish Level 1 to unlock Zip's rescue."));return;}
 MenuSelection=Level;PendingLevel=Level;MenuPage=2;NameEntry=false;MenuLoadTime=Clock+.8f;
}

void ARescueWorld::FinishLevelLoad(){
 const int Level=PendingLevel;PendingLevel=0;MenuLoadTime=-1;MenuPage=0;
 if(Level==2&&!FirstClear){MenuPage=1;return;}
 Settings=false;Paused=false;CarMode=false;CarWiring=false;CarEnginePuzzle=false;
 StageTime=0;Dragging=false;LeverDrag=false;PenDown=false;WireStart=-1;
 for(auto* M:BodyMeshes)M->SetVisibility(true);
 for(auto* P:Parts)P->SetActorHiddenInGame(false);
 if(Level==1){UpdateCarLevel();Reset();}
 else if(Level==2){Stage=0;Reset();Stage=0;OpenCarLevel();}
}

void ARescueWorld::MenuClick(FVector2D P){
 auto Hit=[&](float X,float Y,float Width,float Height){return P.X>=X&&P.X<=X+Width&&P.Y>=Y&&P.Y<=Y+Height;};
 if(Hit(35,25,200,48)){Settings=true;Paused=true;return;}
 if(MenuPage==2)return;
 if(MenuPage==0){
  if(Hit(590,705,420,58)){MenuPage=1;NameEntry=false;return;}
  if(Hit(560,815,480,48))NameEntry=true;
 }else{
  if(Hit(270,300,480,350)){MenuSelection=1;RequestLevel(1);}
  else if(Hit(850,300,480,350)){MenuSelection=2;RequestLevel(2);}
  else if(Hit(610,735,380,48))MenuPage=0;
 }
}

void ARescueHUD::DrawMainMenu(){
 const FLinearColor Cyan(.04f,.85f,1),Gold(1,.58f,.08f),White(.94f,.97f,1),Muted(.53f,.65f,.7f),Panel(.006f,.02f,.035f,.97f);
 auto Art=[&](const TCHAR* Path,float X,float Y,float Width,float Height){
  if(auto* T=LoadObject<UTexture2D>(nullptr,Path))DrawTexture(T,OX+X*Scale,OY+Y*Scale,Width*Scale,Height*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
 };
 Rect(0,0,1600,900,FLinearColor::Black);
 Art(TEXT("/Game/UI/MainMenuArt.MainMenuArt"),0,0,1600,900);
 if(W->MenuPage==0){
  Rect(0,680,1600,220,Panel);Rect(450,685,700,3,Cyan);
  Button(TEXT("PLAY  /  ENTER"),590,705,420);
  Text(TEXT("BUILD. RESCUE. CELEBRATE."),590,775,22,White);
  Button(W->Nickname.IsEmpty()?TEXT("ENGINEER NICKNAME (OPTIONAL) [N]"):W->Nickname+(W->NameEntry?TEXT(" _"):TEXT("")),560,815,480);
 }else if(W->MenuPage==1){
  Rect(0,190,1600,710,Panel);Text(TEXT("CHOOSE YOUR LEVEL"),530,225,34,White);
  const TCHAR* Images[]={TEXT("/Game/UI/LevelOnePreview.LevelOnePreview"),TEXT("/Game/UI/LevelTwoPreview.LevelTwoPreview")};
  for(int I=0;I<2;I++){
   const float X=270+580*I;const bool Unlocked=I==0||W->FirstClear;
   Rect(X,300,480,350,FLinearColor(.025f,.065f,.085f));Rect(X,300,480,4,Unlocked?Cyan:Muted);
   Text(I?TEXT("LEVEL 2  /  ZIP THE CAR"):TEXT("LEVEL 1  /  ROBOT RESCUE"),X+25,320,24,Unlocked?White:Muted);
   Art(Images[I],X+25,365,430,190);
   if(!Unlocked){Rect(X+25,365,430,190,FLinearColor(0,0,0,.7f));Text(TEXT("LOCKED"),X+160,430,32,Gold);}
   Button(Unlocked?TEXT("PLAY LEVEL ")+FString::FromInt(I+1):TEXT("COMPLETE LEVEL 1 TO UNLOCK"),X+25,580,430,Unlocked);
   if(Unlocked&&(I?W->CarEverComplete:W->FirstClear))Text(TEXT("COMPLETED - READY TO PLAY AGAIN"),X+35,630,15,Cyan);
  }
  Text(TEXT("Every play starts a fresh repair. Your unlocks and styles are kept."),420,690,18,Muted);
  Button(TEXT("BACK TO MAIN MENU"),610,735,380);
 }else{
  Rect(0,680,1600,220,Panel);Text(FString::Printf(TEXT("PREPARING LEVEL %d"),W->PendingLevel),585,715,30,White);
  Text(TEXT("Getting your workshop ready..."),605,770,20,Cyan);
  const float Sweep=FMath::Fmod(W->Clock*220.f,590.f);
  Rect(480,820,640,8,FLinearColor(.07f,.18f,.23f));Rect(480+Sweep,820,50,8,Cyan);
 }
 Button(TEXT("SETTINGS"),35,25,200);
}
