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

	class USkeletalMeshComponent* GetArmMesh() const { return ArmMesh; }

	UFUNCTION(Server, Reliable)
	void ServerPickUpWeapon(class AOmwWeapon* Weapon);

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION(Client, Reliable)
	void ClientInit();

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

	void Interacting();
	void StopInteracting();

	void TryToInteract();
	void CheckInteracting();

	void Interact();
	void BeginInteracting();
	void EndInteracting();

	UFUNCTION(Server, Reliable)
	void ServerInteracting(class AOmwInteraction* Target, AOmwCharacter* Interactor, bool bBeginning);

	UFUNCTION(Client, Reliable)
	void ClientStopInteracting();

	UFUNCTION(Server, Reliable)
	void ServerDropOffWeapon();

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
	bool bTryToInteract = false;
	bool bInteracting = false;

	FTimerHandle CheckInteractionTimerHandle;
	FTimerHandle InteractionTimerHandle;

	UPROPERTY(Replicated)
	class AOmwInteraction* Interaction;

	UPROPERTY(Replicated)
	TArray<class AOmwWeapon*> Inventory;

	UPROPERTY(Replicated)
	uint8 InventoryIndex;
};
