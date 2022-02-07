#include "Player/OmwCharacter.h"
#include "Interaction/OmwInteraction.h"
#include "Interaction/OmwWeapon.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AOmwCharacter::AOmwCharacter()
	: SpringArm(CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm")))
	, Camera(CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")))
	, ArmMesh(CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmMesh")))
{
	bAlwaysRelevant = true;
	NetUpdateFrequency = 60.f;
	NetPriority = 4.f;
	SetCanBeDamaged(false);

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionProfileName(TEXT("Pawn"));
	GetMesh()->SetOwnerNoSee(true);

	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeLocation(FVector(20.f, 0.f, 70.f));
	SpringArm->TargetArmLength = 0.f;
	SpringArm->ProbeSize = 5.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 20.f;

	Camera->SetupAttachment(SpringArm);
	Camera->SetRelativeScale3D(FVector(0.25f));

	ArmMesh->SetupAttachment(Camera);
	ArmMesh->SetRelativeLocation(FVector(4.f, 0.f, -165.5f));
	ArmMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	ArmMesh->bSelfShadowOnly = true;
	ArmMesh->bLightAttachmentsAsGroup = true;
	ArmMesh->SetOnlyOwnerSee(true);

	GetCharacterMovement()->MaxWalkSpeed = 360.f;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 170.f;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	Inventory.SetNum(3);
}

void AOmwCharacter::ServerPickUpWeapon_Implementation(AOmwWeapon* Weapon)
{
	if (Weapon && Weapon->GetWeaponType() != EOmwWeaponType::None)
	{
		InventoryIndex = static_cast<uint8>(Weapon->GetWeaponType());

		ServerDropOffWeapon();
		Weapon->SetOwner(this);
		Weapon->ClientTogglePerspective(bUsingFirstPersonView);
		Inventory[InventoryIndex] = Weapon;
	}
}

void AOmwCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	ClientInit();
}

void AOmwCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AOmwCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AOmwCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AOmwCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AOmwCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction(TEXT("TogglePerspective"), IE_Pressed, this, &AOmwCharacter::TogglePerspective);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AOmwCharacter::Jumping);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AOmwCharacter::StopJumping);

	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AOmwCharacter::Crouching);

	PlayerInputComponent->BindAction(TEXT("Run"), IE_Pressed, this, &AOmwCharacter::Running);
	PlayerInputComponent->BindAction(TEXT("Run"), IE_Released, this, &AOmwCharacter::Running);

	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AOmwCharacter::Interacting);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Released, this, &AOmwCharacter::StopInteracting);

	PlayerInputComponent->BindAction(TEXT("DropOffWeapon"), IE_Pressed, this, &AOmwCharacter::ServerDropOffWeapon);
}

void AOmwCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOmwCharacter, bUsingFirstPersonView);
	DOREPLIFETIME(AOmwCharacter, Interaction);
	DOREPLIFETIME(AOmwCharacter, Inventory);
	DOREPLIFETIME(AOmwCharacter, InventoryIndex);
}

void AOmwCharacter::ClientInit_Implementation()
{
	if (auto* World = GetWorld())
		World->GetTimerManager().SetTimer(CheckInteractionTimerHandle, this, &AOmwCharacter::CheckInteracting, 0.1f, true);
}

void AOmwCharacter::MoveForward(float AxisValue)
{
	AddMovementInput(GetActorForwardVector(), AxisValue);
	CheckRunning(!bCrouching && bRunning);
}

void AOmwCharacter::MoveRight(float AxisValue)
{
	AddMovementInput(GetActorRightVector(), AxisValue);
	CheckRunning(!bCrouching && bRunning);
}

void AOmwCharacter::TogglePerspective()
{
	bUsingFirstPersonView = !bUsingFirstPersonView;
	ServerTogglePerspective(bUsingFirstPersonView);

	GetMesh()->SetOwnerNoSee(bUsingFirstPersonView);
	ArmMesh->SetVisibility(bUsingFirstPersonView);

	SpringArm->TargetArmLength = bUsingFirstPersonView ? 0.f : 200.f;
	SpringArm->SetRelativeLocation(FVector(20.f, (bUsingFirstPersonView ? 0.f : 35.f), 70.f));
}

void AOmwCharacter::ServerTogglePerspective_Implementation(bool bInUsingFirstPersonView)
{
	bUsingFirstPersonView = bInUsingFirstPersonView;

	if (auto* Weapon = Inventory[InventoryIndex])
		Weapon->ClientTogglePerspective(bUsingFirstPersonView);
}

void AOmwCharacter::Jumping()
{
	bCrouching ? Crouching() : Jump();
}

void AOmwCharacter::Crouching()
{
	if (GetCharacterMovement()->IsFalling())
		return;

	bCrouching = !bCrouching;
	bCrouching ? Crouch() : UnCrouch();
}

void AOmwCharacter::Running()
{
	bRunning = !bRunning;

	if (bRunning && bCrouching)
	{
		bCrouching = false;
		UnCrouch();
	}
}

void AOmwCharacter::CheckRunning(bool bInRunning)
{
	bInRunning = bInRunning && GetVelocity().Size() > 0.f;
	uint8 RunningShake = static_cast<uint8>(ENwsCameraShakeType::Run);
	GetCharacterMovement()->MaxWalkSpeed = bInRunning ? 540.f : 360.f;

	if (UGameplayStatics::GetPlayerCameraManager(this, 0) && CameraShakeClasses[RunningShake])
	{
		auto* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
		bInRunning ? CameraManager->StartCameraShake(CameraShakeClasses[RunningShake]) : CameraManager->StopAllCameraShakes();
	}

	ServerRunning(bInRunning);
}

void AOmwCharacter::ServerRunning_Implementation(bool bInRunning)
{
	GetCharacterMovement()->MaxWalkSpeed = bInRunning ? 540.f : 360.f;
}

void AOmwCharacter::Interacting()
{
	bTryToInteract = true;

	if (Interaction)
		TryToInteract();
}

void AOmwCharacter::StopInteracting()
{
	bTryToInteract = false;

	if (Interaction)
		EndInteracting();
}

void AOmwCharacter::TryToInteract()
{
	if (!Interaction || !Interaction->CanBeInteracted())
		return;

	BeginInteracting();

	if (auto* World = GetWorld())
	{
		World->GetTimerManager().PauseTimer(CheckInteractionTimerHandle);
		Interaction->GetInteractionTime() ? World->GetTimerManager().SetTimer(InteractionTimerHandle, this, &AOmwCharacter::Interact, Interaction->GetInteractionTime()) : Interact();
	}
}

void AOmwCharacter::CheckInteracting()
{
	if (bRunning)
		Interaction = nullptr;

	else if (auto* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		FHitResult OutHit;
		const auto& Start = CameraManager->GetCameraLocation() + (bUsingFirstPersonView ? FVector(0.f) : CameraManager->GetActorForwardVector() * (CameraManager->GetCameraLocation() - GetActorLocation()).Size());

		if (UKismetSystemLibrary::LineTraceSingle(this, Start, Start + CameraManager->GetActorForwardVector() * 300.f, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>{}, EDrawDebugTrace::None, OutHit, true))
		{
			if (auto* Target = Cast<AOmwInteraction>(OutHit.GetActor()))
			{
				if (Target->CanBeInteracted())
				{
					Interaction = Target;

					if (bTryToInteract)
						TryToInteract();

					return;
				}
			}
		}

		if (Interaction)
			Interaction->EndHover();

		EndInteracting();
		Interaction = nullptr;
	}
}

void AOmwCharacter::Interact()
{
	ServerInteracting(Interaction, this, false);

	bTryToInteract = bInteracting = false;
	Interaction = nullptr;

	if (auto* World = GetWorld())
		World->GetTimerManager().UnPauseTimer(CheckInteractionTimerHandle);
}

void AOmwCharacter::BeginInteracting()
{
	ServerInteracting(Interaction, this, true);

	bTryToInteract = false;
	bInteracting = true;
}

void AOmwCharacter::EndInteracting()
{
	bInteracting = false;

	if (Interaction)
		Interaction->EndInteracting();

	if (auto* World = GetWorld())
	{
		World->GetTimerManager().PauseTimer(InteractionTimerHandle);
		World->GetTimerManager().UnPauseTimer(CheckInteractionTimerHandle);
	}
}

void AOmwCharacter::ServerInteracting_Implementation(AOmwInteraction* Target, AOmwCharacter* Interactor, bool bBeginning)
{
	if (Target && Interactor)
		bBeginning ? Target->BeginInteracting(Interactor) : Target->Interact(Interactor);
}

void AOmwCharacter::ClientStopInteracting_Implementation()
{
	if (bInteracting)
		EndInteracting();
}

void AOmwCharacter::ServerDropOffWeapon_Implementation()
{
	if (!Inventory[InventoryIndex])
		return;

	Inventory[InventoryIndex]->ClientTogglePerspective(false);
	Inventory[InventoryIndex]->SetOwner(nullptr);
	Inventory[InventoryIndex] = nullptr;
}
