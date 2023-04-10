// Fill out your copyright notice in the Description page of Project Settings.


#include "PolyCharacterBase.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/InteractionComp.h"
#include "Weapon/PolyWeaponSwayComp.h"


APolyCharacterBase::APolyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	// 武器与交互组件
	WeaponComp = CreateDefaultSubobject<UPolyWeaponComp>(TEXT("WeaponComp"));
	InteractionComp = CreateDefaultSubobject<UInteractionComp>(TEXT("InteractionComp"));

	// 弹簧臂、镜头
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	if (SpringArmComp)
	{
		SpringArmComp->SetupAttachment(RootComponent);
		SpringArmComp->TargetArmLength = 0.0f;
		SpringArmComp->bUsePawnControlRotation = true;
		SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
		SpringArmComp->bEnableCameraLag = true;
	}
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	if (CameraComp)
	{
		CameraComp->SetupAttachment(SpringArmComp);
	}

	// 网格体组件
	WeaponSwayComp = CreateDefaultSubobject<UPolyWeaponSwayComp>(TEXT("WeaponSwayComp"));
	if (WeaponSwayComp)
	{
		WeaponSwayComp->SetupAttachment(CameraComp);
	}
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetOnlyOwnerSee(true);
		FirstPersonMesh->SetupAttachment(WeaponSwayComp);
		FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	}
	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(true);
	}

	// 移动组件
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->AirControl = 0.8f;
		GetCharacterMovement()->SetWalkableFloorAngle(48.0f);
		GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
		GetCharacterMovement()->GravityScale = 1.5f;
		GetCharacterMovement()->JumpZVelocity = 600.0f;
		GetCharacterMovement()->CrouchedHalfHeight = 42.0f;
	}

	HitPoints = 100.0f;
	MaxHitPoints = 100.0f;

	// 默认移动数据，可以在子类蓝图中修改
	MovementData = TMap<EPolyMovementState, FPolyMovementData>();
	MovementData.Emplace(EPolyMovementState::Walking, FPolyMovementData(450.0f));
	MovementData.Emplace(EPolyMovementState::Sprinting, FPolyMovementData(700.0f));
	MovementData.Emplace(EPolyMovementState::Sliding, FPolyMovementData(1200.0f, 0.0f, 480.0f, 0.0f));
	MovementData.Emplace(EPolyMovementState::Crouching, FPolyMovementData(300.0f));
	MovementData.Emplace(EPolyMovementState::Falling, FPolyMovementData(600.0f, 1024.0f));

	MovementState = EPolyMovementState::Walking;

	DefaultFOV = 80.0f;
	SprintFOV = 87.0f;
	SlideTime = 1.0f;
	TargetFOV = DefaultFOV;
}

void APolyCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APolyCharacterBase, HitPoints, COND_None);
	DOREPLIFETIME_CONDITION(APolyCharacterBase, MaxHitPoints, COND_None);
	DOREPLIFETIME_CONDITION(APolyCharacterBase, MovementState, COND_SkipOwner);
}

void APolyCharacterBase::PawnClientRestart()
{
	// 确保我们具有有效的玩家控制器。
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// 从与我们的玩家控制器相关的本地玩家获取Enhanced Input本地玩家子系统。
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// PawnClientRestart在Actor的生命周期中可以运行多次，因此首先要清除任何残留的映射。
			Subsystem->ClearAllMappings();

			// 添加每个映射上下文及其优先级值。较高的值优先于较低的值。
			Subsystem->AddMappingContext(IMC_Default, 0);
		}
	}
	Super::PawnClientRestart();
}

void APolyCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 设置初始输入状态
	bHoldingCrouchKey = false;
	bHoldingSprintKey = false;
}

// Called when the game starts or when spawned
void APolyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	UpdateMovementData(EPolyMovementState::Walking);
}

void APolyCharacterBase::Move(const FInputActionValue& Value)
{
	WeaponSwayComp->UpdateMoveInput(Value);

	ForwardInput = Value[1];
	RightInput = Value[0];
	if (Value.GetMagnitudeSq() > 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), Value[1]);
		AddMovementInput(GetActorRightVector(), Value[0]);
	}
}

void APolyCharacterBase::View(const FInputActionValue& Value)
{
	WeaponSwayComp->UpdateViewInput(Value);

	if (Value.GetMagnitudeSq() > 0.0f)
	{
		AddControllerPitchInput(-Value[1]);
		AddControllerYawInput(Value[0]);
	}
}

void APolyCharacterBase::StartJumping()
{
	if (MovementState == EPolyMovementState::Sliding or MovementState == EPolyMovementState::Crouching)
	{
		UnCrouch();
	}

	if (CanJump())
	{
		Jump();
	}
}

void APolyCharacterBase::StartSprinting()
{
	if (WeaponComp->IsAiming())
	{
		StopADS();
	}
	bHoldingSprintKey = true;
}

void APolyCharacterBase::ExecuteStartSprinting()
{
	if (MovementState != EPolyMovementState::Falling)
	{
		if (MovementState == EPolyMovementState::Crouching)
		{
			UnCrouch();
		}
		UpdateMovementData(EPolyMovementState::Sprinting);
	}
}

void APolyCharacterBase::StopSprinting()
{
	bHoldingSprintKey = false;
}

void APolyCharacterBase::ExecuteStopSprinting()
{
	if (MovementState == EPolyMovementState::Sprinting)
	{
		UpdateMovementData(EPolyMovementState::Walking);
	}
}

void APolyCharacterBase::StartCrouching()
{
	bHoldingCrouchKey = true;

	if (CanSlide())
	{
		// 开始滑铲
		Crouch();
		UpdateMovementData(EPolyMovementState::Sliding);

		GetCharacterMovement()->AddImpulse( // 滑铲时瞬间加速 TODO: 可能在多人联机存在问题
			GetCharacterMovement()->Velocity.GetSafeNormal() * 240.0f, true);
		GetWorldTimerManager().SetTimer(TimerHandle_Slide, this, &APolyCharacterBase::StopSliding, SlideTime);
	}
	/* Note: CanCrouch() 不覆写的原因是ACharacter内部需要用到CanCrouch() */
	else if (MovementState == EPolyMovementState::Walking and CanCrouch())
	{
		Crouch();
		UpdateMovementData(EPolyMovementState::Crouching);
	}
}

void APolyCharacterBase::StopCrouching()
{
	bHoldingCrouchKey = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_Slide);

	switch (MovementState)
	{
	case EPolyMovementState::Sliding:
		UnCrouch();
		UpdateMovementData(EPolyMovementState::Sprinting);
		break;
	case EPolyMovementState::Crouching:
		UnCrouch();
		UpdateMovementData(EPolyMovementState::Walking);
		break;
	default:
		UnCrouch();
		break;
	}
}

void APolyCharacterBase::StopSliding()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Slide);

	if (MovementState == EPolyMovementState::Sliding)
	{
		UnCrouch();
		UpdateMovementData(EPolyMovementState::Sprinting);
	}
}

void APolyCharacterBase::StopHoldingCrouch()
{
	StopSliding();
}

void APolyCharacterBase::StartADS()
{
	check(WeaponComp);

	WeaponComp->StartADS();
	ExecuteStopSprinting();

	if (MovementState != EPolyMovementState::Sliding and MovementState != EPolyMovementState::Falling)
	{
		// 修改最大速度
		GetCharacterMovement()->MaxWalkSpeed =
			FMath::Min(WeaponComp->GetAdsMovementSpeed(), MovementData[MovementState].MaxSpeed);
	}
}

void APolyCharacterBase::StopADS()
{
	WeaponComp->StopADS();
	GetCharacterMovement()->MaxWalkSpeed = MovementData[MovementState].MaxSpeed;
}

void APolyCharacterBase::StartInspecting()
{
	WeaponComp->StartInspecting();
}

// Called every frame
void APolyCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (auto* PC = GetController<APlayerController>())
	{
		if (PC->IsLocalController())
		{
			// 冲刺逻辑处理
			CheckSprintInput(DeltaTime);

			// 开镜逻辑处理
			if (WeaponComp and WeaponComp->IsAiming())
			{
				PC->InputYawScale = 2.5f * WeaponComp->GetAdsCameraSpeedMod();
				PC->InputPitchScale = -2.5f * WeaponComp->GetAdsCameraSpeedMod();
			}
			else
			{
				// 默认配置文件中的值
				PC->InputYawScale = 2.5f;
				PC->InputPitchScale = -2.5f;
			}

			// FOV
			if (PC->PlayerCameraManager)
			{
				TargetFOV = MovementState == EPolyMovementState::Sprinting
				            or (MovementState == EPolyMovementState::Falling and bHoldingSprintKey)
					            ? SprintFOV
					            : DefaultFOV;

				// Note: 由于冲刺时的FOV与瞄准时的FOV会互相干扰，所以需要手动获取瞄准时的FOV
				if (WeaponComp and WeaponComp->IsAiming())
				{
					TargetFOV = WeaponComp->GetAdsFOV();
				}

				const float NewFOV = FMath::FInterpTo(
					PC->PlayerCameraManager->GetFOVAngle(), TargetFOV, DeltaTime, 10.0f);
				PC->FOV(NewFOV);
			}
		}
	}
}

void APolyCharacterBase::CheckSprintInput(float DeltaTime)
{
	// 同时按住 [冲刺键] 和 [方向键]，且未按住 [射击键] 时可以开始冲刺
	if (bHoldingSprintKey)
	{
		if (ForwardInput or RightInput)
		{
			if (CanSprint())
			{
				ExecuteStartSprinting();
			}
			else if (WeaponComp->GetWeaponState() != EPolyWeaponState::Idle)
			{
				ExecuteStopSprinting();
			}
		}
		else
		{
			// 冲刺时不按住 [方向键] 也会停下来，滑铲时不按方向键不会停下
			if (MovementState == EPolyMovementState::Sprinting)
			{
				ExecuteStopSprinting();
			}
		}
	}
	else
	{
		ExecuteStopSprinting();
	}
}

void APolyCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 角色移动操作
		if (IA_MoveAction)
		{
			EnhancedInputComponent->BindAction(IA_MoveAction, ETriggerEvent::Triggered, this, &APolyCharacterBase::Move);
			EnhancedInputComponent->BindAction(IA_MoveAction, ETriggerEvent::Completed, this, &APolyCharacterBase::Move);
		}
		if (IA_ViewAction)
		{
			EnhancedInputComponent->BindAction(IA_ViewAction, ETriggerEvent::Triggered, this, &APolyCharacterBase::View);
		}
		if (IA_JumpAction)
		{
			EnhancedInputComponent->BindAction(IA_JumpAction, ETriggerEvent::Started, this, &APolyCharacterBase::StartJumping);
			EnhancedInputComponent->BindAction(IA_JumpAction, ETriggerEvent::Completed, this, &APolyCharacterBase::StopJumping);
		}
		if (IA_SprintAction)
		{
			EnhancedInputComponent->BindAction(IA_SprintAction, ETriggerEvent::Started, this, &APolyCharacterBase::StartSprinting);
			EnhancedInputComponent->BindAction(IA_SprintAction, ETriggerEvent::Completed, this, &APolyCharacterBase::StopSprinting);
		}
		if (IA_CrouchAction)
		{
			EnhancedInputComponent->BindAction(IA_CrouchAction, ETriggerEvent::Started, this, &APolyCharacterBase::StartCrouching);
			EnhancedInputComponent->BindAction(IA_CrouchAction, ETriggerEvent::Completed, this, &APolyCharacterBase::StopCrouching);
		}

		// 角色武器操作
		if (WeaponComp)
		{
			// 操作武器
			if (IA_FireAction)
			{
				EnhancedInputComponent->BindAction(IA_FireAction, ETriggerEvent::Started, WeaponComp, &UPolyWeaponComp::StartFiring);
				EnhancedInputComponent->BindAction(IA_FireAction, ETriggerEvent::Completed, WeaponComp, &UPolyWeaponComp::StopFiring);
			}
			if (IA_ReloadAction)
			{
				EnhancedInputComponent->BindAction(IA_ReloadAction, ETriggerEvent::Started, WeaponComp, &UPolyWeaponComp::StartReloading);
			}
			if (IA_AdsAction)
			{
				EnhancedInputComponent->BindAction(IA_AdsAction, ETriggerEvent::Started, this, &APolyCharacterBase::StartADS);
				EnhancedInputComponent->BindAction(IA_AdsAction, ETriggerEvent::Completed, this, &APolyCharacterBase::StopADS);
			}
			if (IA_InspectAction)
			{
				EnhancedInputComponent->BindAction(IA_InspectAction, ETriggerEvent::Started, WeaponComp, &UPolyWeaponComp::StartInspecting);
			}

			// 切换武器
		}

		// 角色交互操作
		if (InteractionComp)
		{
			if (IA_InteractAction)
			{
				EnhancedInputComponent->BindAction(IA_InteractAction, ETriggerEvent::Started, InteractionComp, &UInteractionComp::Interact);
			}
		}
	}
}

bool APolyCharacterBase::CanJumpInternal_Implementation() const
{
	// Note: 主要拷贝自Super::CanJumpInternal_Implementation，
	//		 仅修改部分逻辑使角色可在蹲伏状态直接跳跃。

	if (const UCharacterMovementComponent* CharacterMovement = GetCharacterMovement())
	{
		bool bCanJump = true;

		// Ensure that the CharacterMovement state is valid
		bCanJump &= CharacterMovement->IsMovingOnGround() or CharacterMovement->IsFalling();
		if (bCanJump)
		{
			// Ensure JumpHoldTime and JumpCount are valid.
			if (!bWasJumping || GetJumpMaxHoldTime() <= 0.0f)
			{
				if (JumpCurrentCount == 0 && CharacterMovement->IsFalling())
				{
					bCanJump = JumpCurrentCount + 1 < JumpMaxCount;
				}
				else
				{
					bCanJump = JumpCurrentCount < JumpMaxCount;
				}
			}
			else
			{
				// Only consider JumpKeyHoldTime as long as:
				// A) The jump limit hasn't been met OR
				// B) The jump limit has been met AND we were already jumping
				const bool bJumpKeyHeld = (bPressedJump && JumpKeyHoldTime < GetJumpMaxHoldTime());
				bCanJump = bJumpKeyHeld &&
					((JumpCurrentCount < JumpMaxCount) || (bWasJumping && JumpCurrentCount == JumpMaxCount));
			}
		}
		return bCanJump;
	}

	return false;
}

void APolyCharacterBase::Falling()
{
	UpdateMovementData(EPolyMovementState::Falling);
}

void APolyCharacterBase::Landed(const FHitResult& Hit)
{
	UpdateMovementData(EPolyMovementState::Walking);

	Super::Landed(Hit);
}

void APolyCharacterBase::UpdateMovementData(const EPolyMovementState NewMovementState)
{
	/** 调试代码：显示当前移动状态 */
	if (false and GEngine)
	{
		TMap<EPolyMovementState, FString> DebugNames;
		DebugNames.Emplace(EPolyMovementState::Walking, FString("Walking"));
		DebugNames.Emplace(EPolyMovementState::Sprinting, FString("Sprinting"));
		DebugNames.Emplace(EPolyMovementState::Crouching, FString("Crouching"));
		DebugNames.Emplace(EPolyMovementState::Sliding, FString("Sliding"));
		DebugNames.Emplace(EPolyMovementState::Falling, FString("Falling"));

		GEngine->AddOnScreenDebugMessage(1, 30.0f, FColor::Red, DebugNames[NewMovementState]);
	}

	MovementState = NewMovementState;

	const FPolyMovementData CurrentMovementData = MovementData[NewMovementState];
	GetCharacterMovement()->MaxWalkSpeed = CurrentMovementData.MaxSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CurrentMovementData.MaxSpeed;
	GetCharacterMovement()->MaxAcceleration = CurrentMovementData.MaxAcceleration;
	GetCharacterMovement()->BrakingDecelerationWalking = CurrentMovementData.BrakingDeceleration;
	GetCharacterMovement()->GroundFriction = CurrentMovementData.GroundFriction;

	if (GetLocalRole() != ROLE_Authority)
	{
		ServerUpdateMovementData(NewMovementState);
	}
}

UPolyWeaponComp* APolyCharacterBase::GetWeaponComp() const
{
	return WeaponComp;
}

IInteractionCompInterface* APolyCharacterBase::GetInteractionComp() const
{
	return Cast<IInteractionCompInterface>(InteractionComp);
}

void APolyCharacterBase::ServerUpdateMovementData_Implementation(const EPolyMovementState NewMovementState)
{
	UpdateMovementData(NewMovementState);
}
