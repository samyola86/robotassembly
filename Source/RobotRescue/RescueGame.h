#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "RescueGame.generated.h"

struct FRescueFilament {
 TArray<FVector> Points;
 float Life=0,Duration=.2f,Width=4,Power=1;
};

struct FRescueStyle {
 int32 Paint=0, Face=0, Accessory=0;
};
UCLASS()
class ARobotPart : public AActor {
 GENERATED_BODY()
public:
 ARobotPart();
 int32 Id=0;
 FVector Home, Socket;
 bool Installed=false;
 void Make(int32 Kind);
 class UStaticMeshComponent* Piece(const FString& Shape,FVector Pos,FVector Size,FLinearColor Color,FRotator Rot=FRotator::ZeroRotator,bool Glow=false);
};

UCLASS()
class ARescueWorld : public AActor {
 GENERATED_BODY()
public:
 ARescueWorld();
 virtual void BeginPlay() override;
 virtual void Tick(float Dt) override;
 UPROPERTY() TArray<ARobotPart*> Parts;
 UPROPERTY() TArray<UStaticMeshComponent*> Chains;
 UPROPERTY() TArray<UStaticMeshComponent*> Eyes;
 UPROPERTY() TArray<UStaticMeshComponent*> BodyMeshes;
 UPROPERTY() UStaticMeshComponent* LeverStick;
 UPROPERTY() UStaticMeshComponent* LeverKnob;
 TArray<FVector> BodyRest;
 bool BuildingBody=false;
 bool BatteryBackward=false,TestSaving=false,ShotTaken=false;
 UPROPERTY() class UAudioComponent* Music;
 UPROPERTY() class UAudioComponent* Cheer;
 UPROPERTY() class UAudioComponent* Effects;
 float AliveDuration=0,CheerDuration=0;
 bool YayStarted=false;
 void PlayEffect(const FString& Name);
 void UpdateReleaseAudio();
 UPROPERTY() class UProceduralMeshComponent* Plasma;
 UPROPERTY() class UMaterialInstanceDynamic* PlasmaMaterial;
 UPROPERTY() class UPointLightComponent* PlasmaLight;
 TArray<FRescueFilament> Filaments;
 int32 PlasmaSeed=1,FXCaptureFrame=-1;
 float PlasmaSpawnClock=0;
 void UpdateElectricity(float Dt);
 void AddFilament(FVector Start,FVector End,float Width,float Power,bool Branches);
 float AwakenTime=-1;
 int32 MusicStage=-1;
 void UpdateMusic();
 int32 LastBeat=-1;
 float LeverStartY=0;
 UPROPERTY() class ACameraActor* Camera;
 UPROPERTY() class UAudioComponent* Voice;
 UPROPERTY() class USoundWaveProcedural* Tone;
 UPROPERTY() class UAudioComponent* ToneAudio;
 int32 Stage=0,Selected=-1,WireStart=-1,WireColor=0,Focus=0;
 bool Red=false,Black=false,Paused=false,Quiet=false,Reduced=false,NameEntry=false,Collection=false,Settings=false,Help=false,Dragging=false,LeverDrag=false,FirstClear=false;
 FString Nickname,Line,Task,SaveFile;
 float Clock=0,StageTime=0,Reaction=0,Lever=0,VoiceTime=0;
 int32 ReactionPart=-1, PowerCount=0;
 float PowerCountdown=-1.f;
 bool Challenge=false, CircuitBlocked=false;
 int32 GuidedPad() const;
 void ChooseChallenge(bool Enabled);
 void GiveHint();
 void UpdateRepairReaction(float Dt);
 bool TrackHitsObstacle(FVector2D& At) const;
 void AddRouteWaypoint(FVector2D P);

 // Workshop jobs and robot styles live alongside the main repair checkpoint.
 int32 WorkshopJob=0, JobMedals=0, GearChoice=0, ActiveRobot=0;
 bool CarMode=false, CarComplete=false, CarEverComplete=false;
 int32 MenuPage=0, MenuSelection=1, PendingLevel=0;
 float MenuLoadTime=-1.f;
 void ReturnToMenu();
 void RequestLevel(int32 Level);
 void FinishLevelLoad();
 void MenuClick(FVector2D Position);
 int32 CarInstalled=0, CarSelected=-1;
 int32 CarReturnStage=10, CarCylinders=0, CarPaintIndex=0, CarLook=0;
 int32 CarEngineOffset=2;
 int32 CarOrder[3]={3,1,2}, CarCylinderDrag=-1;
 bool CarPartDrag=false;
 void CarUp(FVector2D Position);
 bool CarEnginePlaced=false, CarEngineAligned=false, CarEnginePuzzle=false;
 bool CarWiring=false, CarWired=false, CarHead=false, CarReleased=false;
 bool CarLampOn=true;
 float CarTravel=0.f, CarTravelTarget=0.f, CarReleaseProgress=0.f;
 float CarTestTime=-1.f;
 int32 CarCelebrationCue=0;
 float CarWheelieAngle=0.f;
 void ReleaseCar();
 void TickCar(float Dt);
 UPROPERTY() UStaticMeshComponent* CarLiftDeck=nullptr;
 UPROPERTY() TArray<UStaticMeshComponent*> CarLiftPistons;
 UPROPERTY() TArray<UStaticMeshComponent*> CarLiftPads;
 UPROPERTY() TArray<UStaticMeshComponent*> CarSmoke;
 UPROPERTY() TArray<UMaterialInstanceDynamic*> CarSmokeMaterials;
 UPROPERTY() TArray<UStaticMeshComponent*> CarBody;
 TArray<FVector> CarBodyRest;
 UPROPERTY() TArray<UStaticMeshComponent*> CarWheels;
 UPROPERTY() TArray<UStaticMeshComponent*> CarHubs;
 UPROPERTY() UStaticMeshComponent* CarBattery=nullptr;
 UPROPERTY() UStaticMeshComponent* CarEngine=nullptr;
 UPROPERTY() UStaticMeshComponent* CarFacePart=nullptr;
 UPROPERTY() TArray<UStaticMeshComponent*> CarTower;
 UPROPERTY() TArray<class UMaterialInstanceDynamic*> CarPaintMaterials;
 UPROPERTY() TArray<class UMaterialInstanceDynamic*> CarLampMaterials;
 UPROPERTY() TArray<class UPointLightComponent*> CarHeadlights;
 void BuildCarLevel();
 void OpenCarLevel();
 void UpdateCarLevel();
 void CarClick(FVector2D Position);
 void CarKey(FKey Key);
 void FitCarPart(int32 Part);
 void CarEngineClick(FVector2D Position);
 int32 PumpStart=-1, PumpSources[2]={-1,-1}, PumpFocus=0;
 float JobTestTime=-1.f;
 bool JobPassed=false;
 FRescueStyle RobotStyles[3];
 FString JobMessage;
 UPROPERTY() UStaticMeshComponent* StyleCap=nullptr;
 UPROPERTY() UStaticMeshComponent* StyleBrim=nullptr;
 UPROPERTY() UStaticMeshComponent* StyleBeacon=nullptr;
 UPROPERTY() TArray<UStaticMeshComponent*> HappyFace;
 UPROPERTY() TArray<UStaticMeshComponent*> StarFace;
 UPROPERTY() TArray<UStaticMeshComponent*> StyleSmile;
 UPROPERTY() TArray<class UMaterialInstanceDynamic*> StylePaint;
 UPROPERTY() TArray<class UMaterialInstanceDynamic*> StyleEyes;
 bool RobotUnlocked(int32 Robot) const;
 void SelectRobot(int32 Robot);
 void CycleStyle(int32 Feature);
 void BuildRobotStyle();
 void ApplyRobotStyle();
 void OpenWorkshopJob(int32 Job);
 void TestWorkshopJob();
 void TapPumpTerminal(int32 Terminal);
 bool ClickWorkshop(FVector2D P);
 bool KeyWorkshop(FKey K);

 TArray<int32> DanceQueue;
 int32 DanceMove=-1;
 float DanceMoveStart=0.f, DanceMoveUntil=0.f, ManualFoilTime=-1.f;
 void StartDanceReplay();
 void QueueDance(int32 Move);
 void FireConfetti();
 void UpdateDanceControls();
 FVector2D Mouse;
 TArray<FVector2D> Tracks[4],PreviousTracks[4],PenPoints;
 int32 TrackStart[4]={-1,-1,-1,-1},TrackEnd[4]={-1,-1,-1,-1},PreviousStart[4]={-1,-1,-1,-1},PreviousEnd[4]={-1,-1,-1,-1};
 int32 ActiveTrack=0;
 bool TracksTouch=false;
 FVector2D TouchPoint;
 bool TrackCollision(FVector2D& At) const;
 bool PenDown=false,Eraser=false,CircuitPassed=false;
 float TestStarted=-100;
 FString MonitorMessage=TEXT("Choose a marker. Follow the schematic.");
 void RecordPen(FVector2D P); void FinishTrack(int32 End); void TestCircuit(); void SendCircuit(); void UndoTrack();
 UPROPERTY() TArray<UStaticMeshComponent*> FinaleFoil;
 UPROPERTY() UProceduralMeshComponent* DiscoTiles=nullptr;
 UPROPERTY() UStaticMeshComponent* DiscoCable=nullptr;
 UPROPERTY() USceneComponent* DiscoRig=nullptr;
 UPROPERTY() TArray<class USpotLightComponent*> DiscoSpots;
 UPROPERTY() TArray<class UPointLightComponent*> DiscoWash;
 void BuildDisco();
 void UpdateDisco();
 void UpdateFinaleFoil();
 int32 Failures=0,XP=0;
 FString Names[9];
 void UpdateDance(); void UpdateLever(); void BuildRoom(); void Refresh(); void Say(const FString& Text,const FString& Clip=TEXT(""));
 bool Available(int32 Id) const; bool Ready() const; bool TryPlace(int32 Id,int32 Target); bool Connect(int32 Color,int32 Start,int32 End);
 void Pick(int32 Id); void Release(); void Click(FVector2D P); void Up(FVector2D P); void Key(FKey Key);
 void Advance(); void Complete(); void Save(); void Load(); void Reset(); void Beep(float Frequency=.0f); void RunTests();
 FVector2D Screen(FVector P) const; FVector World(FVector2D P) const;
 UStaticMeshComponent* Box(FString Shape,FVector Pos,FVector Size,FLinearColor Color,FRotator Rot=FRotator::ZeroRotator,bool Glow=false);
 int32 HitPart(FVector2D P) const; int32 HitSocket(FVector2D P) const; int32 HitTerminal(FVector2D P) const;
 FVector2D Terminal(int32 Id) const;
};

UCLASS()
class ARescueHUD : public AHUD {
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 ARescueWorld* W=nullptr;
 float Scale=1,OX=0,OY=0;
 void Rect(float X,float Y,float Width,float Height,FLinearColor C);
 void Text(FString S,float X,float Y,float Size,FLinearColor C);
 void Stroke(FVector2D A,FVector2D B,FLinearColor C,float Thickness=2);
 void Ring(FVector2D P,float Radius,FLinearColor C,float Thick=2);
 void Button(FString Label,float X,float Y,float Width,bool Active=true);
 void Cable(FVector2D A,FVector2D B,FLinearColor C);
 void DrawAwakening();
 void DrawWorkshop();
 void DrawDanceControls();
 void DrawCarLevel();
 void DrawCarEngine();
 void DrawMainMenu();
};

UCLASS()
class ARescueController : public APlayerController {
 GENERATED_BODY()
public:
 virtual void BeginPlay() override;
 virtual bool InputKey(const FInputKeyEventArgs& Params) override;
 virtual bool InputTouch(const FTouchId TouchId,const ETouchType::Type Type,const FVector2D& TouchLocation,const float Force,const uint64 Timestamp) override;
 bool Touching=false;
 ARescueWorld* FindWorld() const;
 FVector2D Pointer() const;
};

UCLASS()
class ARescueMode : public AGameModeBase {
 GENERATED_BODY()
public: ARescueMode(); virtual void BeginPlay() override;
};
