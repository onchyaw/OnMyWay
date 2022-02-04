#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OmwCharacter.generated.h"

UENUM(BlueprintType)
enum class ENwsCameraShakeType : uint8
{
	Run
};

UCLASS()
class ONMYWAY_API AOmwCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOmwCharacter();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);

	void TogglePerspective();

	UFUNCTION(Server, Reliable)
	void ServerTogglePerspective(bool bInUsingFirstPersonView);

	void Jumping();
	void Crouching();
	void Running();
	void CheckRunning(bool bInRunning);

	UFUNCTION(Server, Reliable)
	void ServerRunning(bool bInRunning);

private:
	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	class USkeletalMeshComponent* ArmMesh;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<class UCameraShakeBase>> CameraShakeClasses;

	UPROPERTY(Replicated)
	bool bUsingFirstPersonView = true;

	bool bCrouching = false;
	bool bRunning = false;
};
