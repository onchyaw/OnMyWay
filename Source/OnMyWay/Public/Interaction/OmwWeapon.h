#pragma once

#include "CoreMinimal.h"
#include "Interaction/OmwInteraction.h"
#include "OmwWeapon.generated.h"

UENUM(BlueprintType)
enum class EOmwWeaponType : uint8
{
	Primary,
	Secondary,
	Melee,
	None
};

UCLASS()
class ONMYWAY_API AOmwWeapon : public AOmwInteraction
{
	GENERATED_BODY()
	
public:
	AOmwWeapon();

	EOmwWeaponType GetWeaponType() const { return WeaponType; }
	int32 GetAmmoNum() const { return AmmoNum; }

	UFUNCTION(Client, Reliable)
	void ClientTogglePerspective(bool bInUsingFirstPersonView);

	virtual void SetOwner(AActor* NewOwner) override;
	virtual void Interact(class AOmwCharacter* Interactor) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY()
	class USceneComponent* Socket;

	UPROPERTY(VisibleDefaultsOnly)
	class USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleDefaultsOnly)
	class USkeletalMeshComponent* ArmMesh;

	UPROPERTY(EditDefaultsOnly)
	EOmwWeaponType WeaponType = EOmwWeaponType::None;

	UPROPERTY(Replicated, EditDefaultsOnly)
	int32 AmmoNum;
};
