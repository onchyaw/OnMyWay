#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OmwInteraction.generated.h"

UCLASS()
class ONMYWAY_API AOmwInteraction : public AActor
{
	GENERATED_BODY()
	
public:
	AOmwInteraction();

	bool CanBeInteracted() const { return bCanBeInteracted; }
	float GetInteractionTime() const { return InteractionTime; }
	const FString& GetDescription() const { return Description; }
	const class UTexture2D* GetIcon() const { return Icon; }

	virtual void Interact(class AOmwCharacter* Interactor) {}

	virtual void BeginHover(class AOmwCharacter* Interactor) {}
	virtual void EndHover() {}

	virtual void BeginInteracting(class AOmwCharacter* Interactor) {}
	virtual void EndInteracting() {}

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated, EditDefaultsOnly)
	bool bCanBeInteracted = true;

	UPROPERTY(EditDefaultsOnly)
	float InteractionTime;

	UPROPERTY(EditDefaultsOnly)
	FString Description;

	UPROPERTY(EditDefaultsOnly)
	class UTexture2D* Icon;
};
