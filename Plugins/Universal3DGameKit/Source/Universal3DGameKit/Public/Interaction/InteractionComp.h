// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Interaction/InteractionCompInterface.h"
#include "InteractionComp.generated.h"

class IInteractiveInterface;

/**
 * 交互能力组件，角色对物品的拾取、与场景机关的交互功能由此组件完成。
 * 交互组件有两种交互模式：范围模式和射线模式。
 * - 范围模式：检测一定范围内的可交互物体，根据距离依次加入可交互列表。
 * - 射线模式：从摄像机中心点往摄像机前方射出射线，检测命中的可交互物体。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNIVERSAL3DGAMEKIT_API UInteractionComp : public USphereComponent, public IInteractionCompInterface
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TArray<AActor*> InteractiveItems;

	/** 使用射线交互模式。第一人称常用的交互模式。 */
	UPROPERTY(EditAnywhere, Category="Game Kit|Interaction|InteractionComp")
	bool bUseTraceMode = false;
	/** 交互范围 */
	UPROPERTY(EditAnywhere, Category="Game Kit|Interaction|InteractionComp")
	float InteractionRange = 200.0f;

	////////////////////////////////////
	/// 射线交互模式相关
	IInteractiveInterface* HitActorCache;
	/** 检测时需要忽略的Actor */
	UPROPERTY()
	TArray<const AActor*> IgnoredActors;
	/** 检测范围，范围越大可使镜头中心离物体越远就能与物体交互 */
	UPROPERTY(EditAnywhere, meta=(DisplayAfter="InteractionRange", EditCondition="bUseTraceMode"))
	float TraceScale = 50.0f;

protected:
	UFUNCTION(Server, Reliable)
	void ServerInteract();

	/** [server] 和可交互列表的第一个进行交互 */
	bool InteractFirstItem();

	void TraceInteractiveItem();

public:
	// Sets default values for this component's properties
	UInteractionComp();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin IInteractionCompInterface
	UFUNCTION()
	virtual void Interact() override;

	virtual void EnterInteractiveItems(AActor* InteractiveActor) override;
	
	virtual void ExitInteractiveItems(AActor* InteractiveActor) override;
};
