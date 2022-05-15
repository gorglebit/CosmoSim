// Copyright Epic Games, Inc. All Rights Reserved.

#include "CosmoSimCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"

//////////////////////////////////////////////////////////////////////////
// ACosmoSimCharacter

ACosmoSimCharacter::ACosmoSimCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	IsTurboActive = false;
	IsBoostActive = false;
	IsBoostActive = false;
}

void ACosmoSimCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Set Aim PitchY and Set AimYawZ value for AimOffset
	auto SetAimPitch = [this]()
	{
		if (IsBoostActive || IsTurboActive) return;

		if (GetLocalRole() == ROLE_Authority)
		{
			const FRotator TargetRotator = UKismetMathLibrary::NormalizedDeltaRotator(
				GetControlRotation(), GetActorRotation());
			AimPitchY = UKismetMathLibrary::Clamp(TargetRotator.Pitch, -90, 90);
		}
	};

	SetAimPitch();

	auto SetAimYaw = [this]()
	{
		if (IsBoostActive || IsTurboActive) return;

		if (GetLocalRole() == ROLE_Authority)
		{
			const FRotator TargetRotator = UKismetMathLibrary::NormalizedDeltaRotator(
				GetControlRotation(), GetActorRotation());
			AimYawZ = UKismetMathLibrary::Clamp(TargetRotator.Yaw, -90, 90);
		}
	};

	SetAimYaw();
	//------------------

	// Set MouseInputX and MouseInputY value for TurboLocomotion blendspace
	auto SetMouseXInput = [this]()
	{
		//if(!IsTurboActive) return;

		MouseInputX = UKismetMathLibrary::Clamp((GetInputAxisValue("Turn Right / Left Mouse") * 10), -1, 1);
	};

	SetMouseXInput();

	auto SetMouseYInput = [this]()
	{
		if (!IsTurboActive) return;

		MouseInputY = UKismetMathLibrary::Clamp((GetInputAxisValue("Look Up / Down Mouse") * 10), -1, 1);
	};

	SetMouseYInput();
	//-----------------

	SetGetUpAnimMontage();
	
	//UE_LOG(LogTemp, Warning, TEXT("Input vector is  - %f , %f"), MovementUnitVector2D.X, MovementUnitVector2D.Y );
	//UE_LOG(LogTemp, Warning, TEXT("Is boost active  - %d"), IsBoostActive );
}

//////////////////////////////////////////////////////////////////////////

// Input
void ACosmoSimCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump / Boost", IE_Pressed, this, &ACosmoSimCharacter::JumpBoostAction);
	PlayerInputComponent->BindAction("Jump / Boost", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Turbo", IE_Pressed, this, &ACosmoSimCharacter::TurboModeAction);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ACosmoSimCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ACosmoSimCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &ACosmoSimCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &ACosmoSimCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ACosmoSimCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ACosmoSimCharacter::TouchStopped);
}

void ACosmoSimCharacter::SetGetUpAnimMontage_Implementation()
{
	if(IsRagdollActive)
	{
		FVector PelvisLocation = GetMesh()->GetSocketLocation("pelvis");
		GetCapsuleComponent()->SetWorldLocation(PelvisLocation);
		
		FVector PelvisRightVector = UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis"));
		FVector Multi = {50, 50, 05};
		FVector EndVector = PelvisLocation + (PelvisRightVector * Multi);
		FHitResult OutHit;
		FCollisionQueryParams Params;
		
		bool Hit = GetWorld()->LineTraceSingleByChannel(OutHit, PelvisLocation, EndVector, ECC_Visibility, Params);

		if(Hit)
		{
			
		}
		else
		{
			
		}
	}
}

void ACosmoSimCharacter::GetUp_Implementation()
{
	
}

void ACosmoSimCharacter::StartRagdoll()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();

	IsRagdollActive = true;
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionProfileName("Ragdoll", true);

	if(IsTurboActive)
		SetTurboModeActive(false);

	if(IsBoostActive)
		SetBoostModeActive(false);
}

void ACosmoSimCharacter::SetBoostModeActive(const bool State)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;

	if (State)
	{
		if(IsTurboActive)
		{
			SetTurboModeActive(false);
		}
		
		if (MovementComponent->IsFalling() && !MovementComponent->IsFlying())
		{
			IsBoostActive = State;
			SetOrientRotationByController(IsBoostActive);

			MovementComponent->SetMovementMode(MOVE_Flying);

			MovementComponent->Velocity = {
				MovementComponent->Velocity.X / BoostBraking,
				MovementComponent->Velocity.Y / BoostBraking,
				0
			};
		}
	}
	else
	{
		if (MovementComponent->IsFlying())
		{
			IsBoostActive = State;
			SetOrientRotationByController(IsBoostActive);

			MovementComponent->SetMovementMode(MOVE_Falling);
		}
	}
}

void ACosmoSimCharacter::SetTurboModeActive(const bool State)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;
	
	if (State)
	{
		if(IsBoostActive)
		{
			SetBoostModeActive(false);
		}
		
		IsTurboActive = State;
		SetOrientRotationByController(IsTurboActive);
		
		MovementComponent->SetMovementMode(MOVE_Flying);
		MovementComponent->MaxFlySpeed = 800;

		UE_LOG(LogTemp, Warning, TEXT("Turbo is -  %i"), IsTurboActive);
	}
	else
	{
		IsTurboActive = State;
		SetOrientRotationByController(IsTurboActive);
		
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxFlySpeed = 600;

		UE_LOG(LogTemp, Warning, TEXT("Turbo is -  %i"), IsTurboActive);
	}
}

void ACosmoSimCharacter::JumpBoostAction()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;

	if (MovementComponent->IsWalking() && !MovementComponent->IsFalling())
	{
		Super::Jump();
		return;
	}

	if (IsBoostActive)
	{
		SetBoostModeActive(false);
	}
	else
	{
		SetBoostModeActive(true);
	}
}

void ACosmoSimCharacter::TurboModeAction()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;

	if (IsTurboActive)
	{
		SetTurboModeActive(false);
	}
	else
	{
		SetTurboModeActive(true);
	}
}

void ACosmoSimCharacter::GetUpWhenMove()
{
	float AxisY = GetInputAxisValue("Move Forward / Backward");

	float AxisX = GetInputAxisValue("Move Right / Left");
	
	const float VelocityLength = UKismetMathLibrary::VSize(GetVelocity());
	UE_LOG(LogTemp, Warning, TEXT("VelocityLength is - %f"), VelocityLength);

	if(AxisX != 0 || AxisY != 0)
	{
		if( VelocityLength == 0 && IsRagdollActive)
		{
			GetUp();
		}
	}
}

void ACosmoSimCharacter::SetOrientRotationByController(const bool IsOrientByController)
{
	bUseControllerRotationYaw = IsOrientByController;
	GetCharacterMovement()->bOrientRotationToMovement = !IsOrientByController;
}

void ACosmoSimCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ACosmoSimCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ACosmoSimCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ACosmoSimCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ACosmoSimCharacter::BeginPlay()
{
	Super::BeginPlay();

	LandedDelegate.AddDynamic(this, &ACosmoSimCharacter::OnLandedEvent);
}

void ACosmoSimCharacter::MoveForward(float Value)
{
	MovementUnitVector2D.Y = Value;

	GetUpWhenMove();
	
	if (Controller != nullptr)
	{
		if (IsTurboActive)
		{
			// get forward vector
			const FVector Direction = FollowCamera->GetForwardVector();
			AddMovementInput(Direction, 1);
		}
		else
		{
			if (Value != 0.0f)
			{
				// find out which way is forward
				const FRotator Rotation = Controller->GetControlRotation();
				const FRotator YawRotation(0, Rotation.Yaw, 0);

				// get forward vector
				const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
				AddMovementInput(Direction, Value);
			}
		}
	}
}

void ACosmoSimCharacter::MoveRight(float Value)
{
	MovementUnitVector2D.X = Value;
	
	if (IsTurboActive) return;

	GetUpWhenMove();
	
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ACosmoSimCharacter::OnLandedEvent(const FHitResult& Hit)
{
	//UE_LOG(LogTemp, Warning, TEXT("OnLanded!"));
	//JumpFlyStateCounter = 0;
}
