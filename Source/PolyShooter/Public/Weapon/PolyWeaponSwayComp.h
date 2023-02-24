// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PolyWeaponSwayComp.generated.h"

struct FInputActionValue;
/**
 * 武器摆动组件
 *  - 用于模拟人物转向时手部摆动的效果，手部模型需要连接到摆动组件上。
 *  - 使用注意：
 *		1. 仅在Owner为Character、实现了WeaponHolderInterface，且控制器为本地玩家控制器时有效。
 *		2. 需要作为Camera的同级组件，并且相对位置、旋转保持为0。
 *		3. 手臂模型需要作为武器摆动组件的子组件。
 */
UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent))
class POLYSHOOTER_API UPolyWeaponSwayComp : public USceneComponent
{
	GENERATED_BODY()
	
	UPROPERTY()
	APlayerController* PlayerController;

	/** 左右摆动速度 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Weapon|WeaponSway", AdvancedDisplay, meta=(AllowPrivateAccess="true"))
	float YawLagMod;
	/** 上下摆动速度 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Weapon|WeaponSway", AdvancedDisplay, meta=(AllowPrivateAccess="true"))
	float PitchLagMod;
	/** 手臂摆动恢复速率。 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Weapon|WeaponSway", meta=(AllowPrivateAccess="true"))
	float RecoveryRate;
	/** 左右摆动最大角度，为0时不左右摆动。 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Weapon|WeaponSway", meta=(AllowPrivateAccess="true"))
	float MaxYawLag;
	/** 上下摆动最大角度，为0时不上下摆动。 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Weapon|WeaponSway", meta=(AllowPrivateAccess="true"))
	float MaxPitchLag;
	
	/** 左右移动时手臂偏转最大角度，为0时不偏转。 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Weapon|WeaponSway", meta=(AllowPrivateAccess="true"))
	float MaxRollLag;
	/** 左右移动时手臂偏转速度。 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Weapon|WeaponSway", AdvancedDisplay, meta=(AllowPrivateAccess="true"))
	float RollLagMod;


	FRotator TargetRotation;
	// 用来记录鼠标输入轴值的变量
	FVector2D ViewAxis;
    // 用来记录移动输入轴值的变量
    FVector2D MoveAxis;

public:
	// Sets default values for this component's properties
	UPolyWeaponSwayComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 用于传入玩家输入轴值，由持有者调用。
	 *  TODO: 改进扩展性，目前将该组件添加到角色中时需要角色完成额外的工作，具有侵入性。 */
	void UpdateViewInput(const FInputActionValue& NewViewAxis);
	void UpdateMoveInput(const FInputActionValue& NewMoveAxis);
};
