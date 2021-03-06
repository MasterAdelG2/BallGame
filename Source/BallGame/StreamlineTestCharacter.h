// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StreamlineTestCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;
class UAudioComponent;

UCLASS(config=Game)
class AStreamlineTestCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;

	/** Gun mesh: VR view (attached to the VR controller directly, no arm, just the actual gun) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* VR_Gun;

	/** Location on VR gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* VR_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Motion controller (right hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* R_MotionController;

	/** Motion controller (left hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* L_MotionController;

public:
	AStreamlineTestCharacter();

protected:
	virtual void BeginPlay();

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AStreamlineTestProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint8 bUsingMotionControllers : 1;

protected:
	
	/** Fires a projectile. */
	void OnFire();

	/** Resets HMD orientation and position in VR. */
	void OnResetVR();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	struct TouchData
	{
		TouchData() { bIsPressed = false;Location=FVector::ZeroVector;}
		bool bIsPressed;
		ETouchIndex::Type FingerIndex;
		FVector Location;
		bool bMoved;
	};
	void BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location);
	TouchData	TouchItem;
	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/* 
	 * Configures input for touchscreen devices if there is a valid touch interface for doing so 
	 *
	 * @param	InputComponent	The input component pointer to bind controls to
	 * @returns true if touch controls were enabled.
	 */
	bool EnableTouchscreenMovement(UInputComponent* InputComponent);

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

// My Added Section of Code
protected:
	// Tick Event for Movement, Jetting & Dashing Application
	void Tick(float DeltaTime);
	// Added Movement Throttling Multiplyed with Move Speed
	float MoveForwardThrottle=0;
	float MoveRightThrottle=0;
	float MoveSpeed=200.f;

// Dashing Part
	// Starts Dash on Next Tick
	bool bDashOrder=false;
	// Prevent any Movement or Flying from Happening
	bool bIsDashing=false;

	// Dash Speed
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "Dashing")
	float DashSpeed= 1000.f;
	// Dash Distance
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "Dashing")
	float DashDistance= 300.f;
	// Dash Time
	float DashTime= 1.f;
	// Dash Direction
	FVector DashDirection= FVector::ZeroVector;
	// Actor Location Before Dashing
	FVector StartDashLocation;
	// Actor Location After Dashing
	FVector EndDashLocation;
	// World Time at Dash Start
	float StartDashTime;
	// World Time at Dash End
	float EndDashTime;
	// Dash Hight
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "Dashing")
	float DashHight= 250.f;
	// TimerHandle for Dashing After elevation
	FTimerHandle ApplyDashTimerHandle;
	// Apply Dash
	UFUNCTION()
	void Dash();
	// Triggers DashOrder to Dash on Next Tick
	UFUNCTION()
	void PreDash();

// Gravity Gun Part
	// Acts like a Hand to Hold Any Object
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "GravGun")
	class UStaticMeshComponent* HeldSlot;
	// Constrain Links the HeldSlot with the GrabbedObject
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "GravGun")
	class UPhysicsConstraintComponent* GrabConstraint;
	// Reference to Grabbed Object
	class UPrimitiveComponent* GrabedObject;
	// Max Allowed Grabbing Range
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "GravGun")
	float GrabRange = 5000.f;
	// Shooting Power
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "GravGun")
	float ShootPower = 1000000.f;
	// Grab Sound Effect
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "GravGun")
	USoundBase* GrabSFX;
	// Try Grab Targeted Object
	UFUNCTION()
	void OnGrab();
	// Draw Line Trace to the Max GrabRange
	UFUNCTION()
	bool TraceObject(FHitResult & Hit);
	// Grabs Hitted Object
	UFUNCTION()
	void GrabObject(FHitResult Hit);
	// Breaks Constrain with GrabbedObject 
	UFUNCTION()
	void DropObject();
	// Apply Force to Object
	UFUNCTION()
	void ShootObject(FHitResult Hit);

// JetBack Part
	// Trigger for Jetting
	bool bIsJetting = false;
	// JetBack Flying Power
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "JetBack")
	float JetPower = 2000.f;
	// Triggers Jetting
	void Jetting();
	// Stops Jetting Trigger
	void StoppedJetting();
	// Jetting Sound Effect
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "JetBack")
	UAudioComponent* JettingSFXSource;
};