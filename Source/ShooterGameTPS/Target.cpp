// Fill out your copyright notice in the Description page of Project Settings.


#include "Target.h"

// Sets default values
ATarget::ATarget()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	box = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));

	RootComponent = box;
	//	SetRootComponent(box);
	
	//	ÎïÀíÄ£Äâ
	box->SetSimulatePhysics(true);
	box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	box->OnComponentBeginOverlap.AddDynamic(this, &ATarget::OnTestHit);

}

// Called when the game starts or when spawned
void ATarget::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATarget::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATarget::OnTestHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Overlap)
{
	//if (OtherActor->GetFName() != FName("ProjectileCPP")) return;
	/*if (OtherActor->GetFName() != FName("ThirdPersonCharacter_2")) return;
	box->AddImpulse(FVector(1000000.0, 0.0, 0.0));*/
}	