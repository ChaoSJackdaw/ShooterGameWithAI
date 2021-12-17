// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameTPSCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "ProjectileCPP.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"

//////////////////////////////////////////////////////////////////////////
// AShooterGameTPSCharacter

AShooterGameTPSCharacter::AShooterGameTPSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bToss = false;
	bShoot = false;
	CounterTime = 0.f;
	num = 1;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = true; // Camera does not rotate relative to arm

	GunOffset = FVector(100.0f, 0.0f, 10.0f);
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(GetMesh());

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));
}

//////////////////////////////////////////////////////////////////////////
// Input
void AShooterGameTPSCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{

	SpeedVector.Set(0.0f, 0.0f, 0.0f);
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterGameTPSCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AShooterGameTPSCharacter::StopJumping);

	//	绑定开火
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AShooterGameTPSCharacter::OnFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AShooterGameTPSCharacter::CancelFire);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterGameTPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterGameTPSCharacter::MoveRight);

	PlayerInputComponent->BindAction("CancelForward", IE_Released,this, &AShooterGameTPSCharacter::CancelForward);
	PlayerInputComponent->BindAction("CancelRight", IE_Released, this, &AShooterGameTPSCharacter::CancelRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterGameTPSCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterGameTPSCharacter::LookUpAtRate);

	//	投掷手雷
	PlayerInputComponent->BindAction("Toss", IE_Pressed, this, &AShooterGameTPSCharacter::Toss);
}

void AShooterGameTPSCharacter::Tick(float deltaTime)
{
	//	设置手雷投掷的间隔，根据Toss动画决定
	if (bToss)
	{
		CounterTime += deltaTime;
		if (CounterTime >= 2.f && num > 0)
		{
			if (Grenade != nullptr)
			{
				UWorld* const World = GetWorld();
				if (World != nullptr)
				{
					const FRotator SpawnRotation = GetControlRotation() + FRotator(20.0f, 0.f, 0.f);
					// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
					const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

					//Set Spawn Collision Handling Override
					FActorSpawnParameters ActorSpawnParams;
					ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

					// spawn the projectile at the muzzle
					World->SpawnActor<AActor>(Grenade, SpawnLocation, SpawnRotation, ActorSpawnParams);
					--num;
				}
			}
		}
		if (CounterTime >= 3.2f)
		{
			CounterTime = 0.f;
			bToss = false;
			FP_Gun->SetVisibility(true);
			++num;
		}
	}
}

void AShooterGameTPSCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());

	Controller->GetControlRotation();
	

}

void AShooterGameTPSCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterGameTPSCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		//	get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		AddMovementInput(Direction, Value);

		SpeedVector.Set(Value * 600.0f, SpeedVector.Y, SpeedVector.Z);
	}
}

void AShooterGameTPSCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		//add movement in that direction

		AddMovementInput(Direction, Value);

		SpeedVector.Set(SpeedVector.X, 600.0f * Value, SpeedVector.Z);
	}
}

void AShooterGameTPSCharacter::CancelForward()
{
	SpeedVector.Set(0.0f, SpeedVector.Y, SpeedVector.Z);
}

void AShooterGameTPSCharacter::CancelRight()
{
	SpeedVector.Set(SpeedVector.X, 0.0f, SpeedVector.Z);
}

FVector AShooterGameTPSCharacter::GetSpeedVector() const
{
	return SpeedVector;
}

void AShooterGameTPSCharacter::Jump()
{
	Super::Jump();
}

void AShooterGameTPSCharacter::StopJumping()
{
	Super::StopJumping();
}

void AShooterGameTPSCharacter::OnFire()
{
	//	Weapon -> shoot bullet
	if (Bullet != nullptr && FP_Gun->GetVisibleFlag())
	{
		bShoot = true;
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			const FRotator SpawnRotation = GetControlRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			// spawn the projectile at the muzzle
			AProjectileCPP* instance = World->SpawnActor<AProjectileCPP>(Bullet, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
		if (FireSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		}
	}
}

void AShooterGameTPSCharacter::CancelFire()
{
	bShoot = false;
}

void AShooterGameTPSCharacter::Toss()
{
	if (bToss) return;
	FP_Gun->SetVisibility(false);
	bToss = true;
}

bool AShooterGameTPSCharacter::SetTossState(bool Toss)
{
	bToss = Toss;
	return Toss;
}

bool AShooterGameTPSCharacter::GetTossState() const
{
	return bToss;
}

bool AShooterGameTPSCharacter::GetShootState() const
{
	return bShoot;
}
