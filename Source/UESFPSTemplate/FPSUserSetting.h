// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FPSUserSetting.generated.h"

/**
 * Persists FPS control settings (mouse sensitivity) between game sessions.
 */
UCLASS()
class UESFPSTEMPLATE_API UFPSUserSetting : public USaveGame
{
	GENERATED_BODY()

public:
	UFPSUserSetting();

	static const FString SaveSlotName;
	static constexpr float DefaultMouseSensitivity = 0.07f;
	static constexpr float MinMouseSensitivity = 0.01f;
	static constexpr float MaxMouseSensitivity = 2.0f;

	UPROPERTY(VisibleAnywhere, Category = "Mouse")
	float MouseSensitivity;

	/** Loads saved settings or creates defaults, then applies mouse sensitivity. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Settings")
	static UFPSUserSetting* LoadOrCreateSettings();

	UFUNCTION(BlueprintCallable, Category = "FPS|Settings")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "FPS|Settings")
	void SetMouseSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintPure, Category = "FPS|Settings")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	/** Applies MouseSensitivity to the engine MouseX/MouseY axis config. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Settings")
	void ApplyMouseSensitivity();

	static void LoadOnStartup();
private:
	

	static float ClampSensitivity(float Sensitivity);
};
