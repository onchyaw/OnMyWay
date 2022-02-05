#include "Interaction/OmwInteraction.h"

#include "Net/UnrealNetwork.h"

AOmwInteraction::AOmwInteraction()
{
	bAlwaysRelevant = true;
	SetReplicateMovement(true);
	bReplicates = true;
}

void AOmwInteraction::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOmwInteraction, bCanBeInteracted);
}
