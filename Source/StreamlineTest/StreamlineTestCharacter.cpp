// Copyright Epic Games, Inc. All Rights Reserved.

#include "StreamlineTestCharacter.h"
#include "StreamlineTestProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
// My Included Libraries
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AStreamlineTestCharacter

AStreamlineTestCharacter::AStreamlineTestCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;

	// My Added Construction Code
	PrimaryActorTick.bCanEverTick = true;

	HeldSlot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldObjectSlot"));
	HeldSlot->SetupAttachment(FirstPersonCameraComponent);

	GrabConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("GrabConstraint"));
	GrabConstraint->SetupAttachment(HeldSlot);

}

void AStreamlineTestCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
}

void AStreamlineTestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsDashing)
	{
		FVector MoveDirection = GetActorForwardVector()* MoveForwardThrottle + GetActorRightVector()* MoveRightThrottle;
		MoveDirection*= DeltaTime;
		// Apply JetBack Force if Pressed
		if (bIsJetting)	
		{
			MoveDirection.Z = JetPower*DeltaTime;
			LaunchCharacter(MoveDirection,false,false);
		}
		if (MoveForwardThrottle || MoveRightThrottle)
		{
			// Apply Movement if Not have Dash Order
			if (!bDashOrder)	
			{
				AddMovementInput(MoveDirection * MoveSpeed);
			}
			// Applying Dash if Not Jetting
			else if (!bIsJetting && !GetCharacterMovement()->IsFalling())
			{
				DashDirection = MoveForwardThrottle ? GetActorForwardVector() * MoveForwardThrottle : GetActorRightVector()* MoveRightThrottle;
				EndDashLocation = GetActorLocation() + DashDirection * DashDistance;
				LaunchCharacter(FVector(0,0,DashHight),false,false);
				GetWorldTimerManager().SetTimer(ApplyDashTimerHandle,this,&AStreamlineTestCharacter::Dash,0.1f,false);				
			}
		}
	}
	// if isDashing Get Current Needed Location or Stop Dashing
	else
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime<=EndDashTime)
		{
			float Alfa = (CurrentTime - StartDashTime) / DashTime;
			FVector CurrentLocation= FMath::Lerp(StartDashLocation,EndDashLocation,Alfa);
			SetActorLocation(CurrentLocation,true);
		}
		else
		{
			bIsDashing = false;
		}
	}
	// set DashOrder back to false
	bDashOrder = false;
}
//////////////////////////////////////////////////////////////////////////
// Input

void AStreamlineTestCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AStreamlineTestCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AStreamlineTestCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AStreamlineTestCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AStreamlineTestCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AStreamlineTestCharacter::LookUpAtRate);
	
	// My Added Binding Keys
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AStreamlineTestCharacter::PreDash);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AStreamlineTestCharacter::OnFire);
	PlayerInputComponent->BindAction("Grab", IE_Pressed, this, &AStreamlineTestCharacter::OnGrab);
	PlayerInputComponent->BindAction("Jetting", IE_Pressed, this, &AStreamlineTestCharacter::Jetting);
	PlayerInputComponent->BindAction("Jetting", IE_Released, this, &AStreamlineTestCharacter::StoppedJetting);
}

void AStreamlineTestCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AStreamlineTestCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AStreamlineTestCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AStreamlineTestCharacter::MoveForward(float Value)
{
	MoveForwardThrottle = Value;
}

void AStreamlineTestCharacter::MoveRight(float Value)
{
	MoveRightThrottle = Value;
}

void AStreamlineTestCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AStreamlineTestCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AStreamlineTestCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AStreamlineTestCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AStreamlineTestCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AStreamlineTestCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}


void AStreamlineTestCharacter::Dash()
{
	StartDashLocation = GetActorLocation();
	DashTime = DashDistance / DashSpeed;
	StartDashTime = GetWorld()->GetTimeSeconds();
	EndDashTime = StartDashTime + DashTime;
	bIsDashing=true;
}

// Triggers DashOrder to Dash on Next Tick
void AStreamlineTestCharacter::PreDash()
{
	bDashOrder = true;
}

void AStreamlineTestCharacter::OnGrab()
{
	if (GrabedObject != nullptr)
	{
		DropObject();
	}
	else
	{
		FHitResult Hit;
		if (TraceObject(Hit))
		{
			GrabObject(Hit);
		}
	}
}

bool AStreamlineTestCharacter::TraceObject(FHitResult &Hit)
{
	FVector StartLocation = FirstPersonCameraComponent->GetComponentLocation();
	FVector EndLocation = StartLocation + FirstPersonCameraComponent->GetForwardVector()* GrabRange;
	bool bSuccess= GetWorld()->LineTraceSingleByObjectType(
	OUT Hit,
	StartLocation,
	EndLocation,
	FCollisionObjectQueryParams(ECollisionChannel::ECC_PhysicsBody));
	return bSuccess;
}

void AStreamlineTestCharacter::GrabObject(FHitResult Hit)
{
	UPrimitiveComponent* HittedComponent= Hit.GetComponent();
	FVector HittedComponentLocation= HittedComponent->GetComponentLocation();
	HeldSlot->SetWorldLocation(HittedComponentLocation);
	GrabConstraint->SetConstrainedComponents(HeldSlot,FName::FName(), HittedComponent,Hit.BoneName);
	Hit.Component->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,ECollisionResponse::ECR_Ignore);
	GrabedObject = Hit.GetComponent();
	FVector GunGrabPoint = FirstPersonCameraComponent->GetComponentLocation() + FirstPersonCameraComponent->GetForwardVector() * 250;
	HeldSlot->SetWorldLocation(GunGrabPoint);
}

void AStreamlineTestCharacter::DropObject()
{
	GrabConstraint->BreakConstraint();
	GrabedObject->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,ECollisionResponse::ECR_Block);
	GrabedObject = nullptr;
}

void AStreamlineTestCharacter::OnFire()
{
	if (GrabedObject != nullptr)
	{
		FHitResult Hit;
		Hit.Component = GrabedObject;
		Hit.ImpactPoint = GrabedObject->GetComponentLocation();
		DropObject();
		ShootObject(Hit);
	}
	else
	{
		FHitResult Hit;
		if (TraceObject(Hit))
		{
			ShootObject(Hit);
		}
	}
}

void AStreamlineTestCharacter::ShootObject(FHitResult Hit)
{
	FVector AppliedForce = FirstPersonCameraComponent->GetForwardVector()*ShootPower;
	Hit.GetComponent()->AddImpulseAtLocation(AppliedForce,Hit.ImpactPoint,Hit.BoneName);
	
	// try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
	// try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AStreamlineTestCharacter::Jetting()
{
	bIsJetting = true;
}

void AStreamlineTestCharacter::StoppedJetting()
{
	bIsJetting = false;
}