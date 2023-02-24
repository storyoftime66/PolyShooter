// Copyright Epic Games, Inc. All Rights Reserved.

#include "Universal3DGameKit.h"

#define LOCTEXT_NAMESPACE "FUniversal3DGameKitModule"

void FUniversal3DGameKitModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FUniversal3DGameKitModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUniversal3DGameKitModule, Universal3DGameKit)