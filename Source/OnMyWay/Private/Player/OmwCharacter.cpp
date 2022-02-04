#include "Player/OmwCharacter.h"

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
}

void AOmwCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOmwCharacter, bUsingFirstPersonView);
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
