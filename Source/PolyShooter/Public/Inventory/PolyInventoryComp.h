// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <fstream>
#include <iostream>
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PolyInventoryComp.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class POLYSHOOTER_API UPolyInventoryComp : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPolyInventoryComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	std::ifstream ifs;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
