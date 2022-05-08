// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CosmoSimCharacter.generated.h"


UCLASS(config=Game)
class ACosmoSimCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	ACosmoSimCharacter();

	virtual void Tick(float DeltaTime) override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

protected:
	virtual void BeginPlay() override;
	
	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

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

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector2D MovementUnitVector2D;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool IsBoostActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool IsTurboActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool IsAimActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AimPitchY;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AimYawZ;
	
	private:
	const float BoostBraking = 1.5;
	int32 JumpFlyStateCounter;
	FTimerHandle FlyingModeTimerHandle;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	void ActivateBoostMode();
	void ActivateTurboMode();

	void SetOrientRotationByController(const bool IsOrientByController);
	
	void SetBoostState(const bool InState);
	void SetTurboState(const bool InState);

	UFUNCTION()
	void OnLandedEvent(const FHitResult& Hit);
};



