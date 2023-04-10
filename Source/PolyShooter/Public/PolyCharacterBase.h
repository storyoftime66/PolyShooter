// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/PolyWeaponComp.h"
#include "Weapon/PolyWeaponHolderInterface.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interaction/InteractionAgentInterface.h"
#include "PolyCharacterBase.generated.h"

class UInputMappingContext;
struct FInputActionValue;
class UInputAction;
class USpringArmComponent;
class UPolyWeaponSwayComp;
class UInteractionComp;
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHitPointChanged, float CurrentHitPoints, float MaxHitPoints);


/**
 * 角色移动状态，PolyCharacterBase使用。
 */
UENUM(BlueprintType)
enum class EPolyMovementState : uint8
{
	Walking,
	Crouching,
	Sprinting,
	Falling,
	Sliding
};

/**
 * 角色特定移动状态下的移动属性
 * 主要包括：最大速度、最大加速度、制动加速度等
 */
USTRUCT()
struct FPolyMovementData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float MaxSpeed;

	UPROPERTY(EditDefaultsOnly)
	float MaxAcceleration;

	UPROPERTY(EditDefaultsOnly)
	float BrakingDeceleration;

	UPROPERTY(EditDefaultsOnly)
	float GroundFriction;

	FPolyMovementData(
		float InMaxSpeed = 600.0f, float InMaxAcceleration = 2048.0f,
		float InBrakingDeceleration = 2048.0f, float InGroundFriction = 8.0):
		MaxSpeed(InMaxSpeed), MaxAcceleration(InMaxAcceleration),
		BrakingDeceleration(InBrakingDeceleration), GroundFriction(InGroundFriction)
	{
	}
};

/**
 * 角色基类
 * 主要处理血量、移动相关逻辑。
 */
UCLASS()
class POLYSHOOTER_API APolyCharacterBase : public ACharacter, public IPolyWeaponHolderInterface, public IInteractionAgentInterface
{
private:
	GENERATED_BODY()

private:
	////////////////////////////////////////////////////////
	// 相机
	UPROPERTY(EditAnywhere, Category="Components")
	USpringArmComponent* SpringArmComp;
	UPROPERTY(EditAnywhere, Category="Components")
	UCameraComponent* CameraComp;

	////////////////////////////////////////////////////////
	// 第一人称网格体
	UPROPERTY(EditAnywhere, Category="Components")
	USkeletalMeshComponent* FirstPersonMesh;

	////////////////////////////////////////////////////////
	// 武器系统
	UPROPERTY(EditAnywhere, Category="Components")
	UPolyWeaponComp* WeaponComp;
	UPROPERTY(EditAnywhere, Category="Components", meta=(AllowPrivateAccess="true"))
	UPolyWeaponSwayComp* WeaponSwayComp;

	////////////////////////////////////////////////////////
	// 交互系统
	UPROPERTY(EditAnywhere, Category="Components")
	UInteractionComp* InteractionComp;

	////////////////////////////////////////////////////////
	// 角色状态
	UPROPERTY(Replicated)
	float HitPoints;
	UPROPERTY(Replicated)
	float MaxHitPoints;
	UPROPERTY(Replicated)
	EPolyMovementState MovementState;

	////////////////////////////////////////////////////////
	// 角色移动数据
	/** 角色各状态下的移动数据 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Movement")
	TMap<EPolyMovementState, FPolyMovementData> MovementData;
	/** 默认情况下的FOV */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Character")
	float DefaultFOV;
	/** 冲刺时的FOV */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Character")
	float SprintFOV;
	/** 最大滑行时间，单位秒 */
	UPROPERTY(EditAnywhere, Category="PolyShooter|Character")
	float SlideTime;

	////////////////////////////////////////////////////////
	// 内部变量
	float TargetFOV; // 目标FOV
	FTimerHandle TimerHandle_Slide;
	FTimerHandle TimerHandle_SprintToFireDelay;
	FTimeline AdsFOVTimeline;

#pragma region INPUT
	////////////////////////////////////////////////////////
	// 玩家输入相关变量
	float ForwardInput;
	float RightInput;
	bool bHoldingSprintKey;
	bool bHoldingCrouchKey;
	bool bHoldingFireKey;

	////////////////////////////////////////////////////////
	// 移动操作
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputMappingContext* IMC_Default;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_MoveAction;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_ViewAction;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_JumpAction;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_SprintAction; // 暂未使用
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_SprintToggleAction;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_CrouchAction; // 暂未使用
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_CrouchToggleAction;

	////////////////////////////////////////////////////////
	// 武器操作
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_FireAction;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_ReloadAction;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_AdsAction;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_InspectAction;

	////////////////////////////////////////////////////////
	// 交互操作
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Input")
	UInputAction* IA_InteractAction;
#pragma endregion

public:
	APolyCharacterBase();

	//~ AActor interfaces
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ APawn interfaces
	virtual void PawnClientRestart() override;

	//~ ACharacter interfaces
	virtual void Falling() override;
	virtual void Landed(const FHitResult& Hit) override;

protected:
	//~ AActor interfaces
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

#pragma region INPUT_HANDLER
	////////////////////////////////////////////////////////
	// 处理移动输入的方法
	void Move(const FInputActionValue& Value);
	void View(const FInputActionValue& Value);

	void StartJumping();
	void StartSprinting();
	void StopSprinting();

	void ExecuteStartSprinting(); // 工具方法
	void ExecuteStopSprinting(); // 工具方法

	void StartCrouching();
	void StopCrouching();
	void StopHoldingCrouch();

	void StopSliding();

	////////////////////////////////////////////////////////
	// 处理武器输入的方法
	void StartADS();
	void StopADS();
	void StartInspecting();

#pragma endregion

public:
	virtual void Tick(float DeltaTime) override;

	//~ IWeaponHolderInterface
	virtual UPolyWeaponComp* GetWeaponComp() const override;

	//~ IInteractionAgentInterface
	virtual IInteractionCompInterface* GetInteractionComp() const override;

	/** 按下冲刺时，执行冲刺操作。 */
	void CheckSprintInput(float DeltaTime);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 是否可进入某个移动状态
	FORCEINLINE bool CanSprint() const { return MovementState == EPolyMovementState::Walking and WeaponComp->GetWeaponState() == EPolyWeaponState::Idle and !WeaponComp->IsAiming(); }
	FORCEINLINE bool CanSlide() const { return MovementState == EPolyMovementState::Sprinting and CanCrouch(); }
	virtual bool CanJumpInternal_Implementation() const override;

	// 组件
	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	FORCEINLINE USkeletalMeshComponent* GetThirdPersonMesh() const { return GetMesh(); }
	FORCEINLINE USkeletalMeshComponent* GetMeshByRole() const
	{
		return IsLocallyControlled() and IsPlayerControlled() ? FirstPersonMesh : GetMesh();
	}

	FORCEINLINE UCameraComponent* GetCamera() const { return CameraComp; }
	UFUNCTION(BlueprintCallable, Category="PolyShooter|Weapon")

	// Animation Blueprint
	UPolyWeaponComp* K2Node_GetWeaponComp() const { return WeaponComp; }

	UFUNCTION(BlueprintCallable, Category="PolyShooter|Movement")
	EPolyMovementState GetMovementState() const { return MovementState; }

	// 更新状态
	UFUNCTION()
	void UpdateMovementData(const EPolyMovementState NewMovementState);
	UFUNCTION(Server, Reliable)
	void ServerUpdateMovementData(const EPolyMovementState NewMovementState);

#if WITH_EDITOR
	void LogDebugMessage(FString DebugMessage) const
	{
		UE_LOG(LogTemp, Display, TEXT("[%s] %s"), *GetName(), *DebugMessage);
	}

	void LogDebugMessage(bool DebugVal) const
	{
		const FString DebugMessage = DebugVal ? TEXT("true") : TEXT("false");
		UE_LOG(LogTemp, Display, TEXT("[%s] %s"), *GetName(), *DebugMessage);
	}

	template <class T>
	void LogDebugMessage(T DebugVal) const
	{
		float DebugValFloat = static_cast<float>(DebugVal);
		UE_LOG(LogTemp, Display, TEXT("[%s] %f"), *GetName(), DebugValFloat);
	}
#endif
};
