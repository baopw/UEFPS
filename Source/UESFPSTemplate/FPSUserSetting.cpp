// Fill out your copyright notice in the Description page of Project Settings.

#include "FPSUserSetting.h"

#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"

const FString UFPSUserSetting::SaveSlotName(TEXT("FPSUserSettings"));

namespace
{
	struct FFPSUserSettingsStartupLoader
	{
		FFPSUserSettingsStartupLoader()
		{
			FCoreDelegates::OnPostEngineInit.AddStatic(&UFPSUserSetting::LoadOnStartup);
		}
	};

	static FFPSUserSettingsStartupLoader GFPSUserSettingsStartupLoader;
}

UFPSUserSetting::UFPSUserSetting()
{
	MouseSensitivity = DefaultMouseSensitivity;
}

UFPSUserSetting* UFPSUserSetting::LoadOrCreateSettings()
{
	UFPSUserSetting* Settings = nullptr;

	if (USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0))
	{
		Settings = Cast<UFPSUserSetting>(LoadedGame);
	}

	if (!Settings)
	{
		Settings = Cast<UFPSUserSetting>(
			UGameplayStatics::CreateSaveGameObject(UFPSUserSetting::StaticClass()));
	}

	if (Settings)
	{
		Settings->MouseSensitivity = ClampSensitivity(Settings->MouseSensitivity);
		Settings->ApplyMouseSensitivity();
	}

	return Settings;
}

void UFPSUserSetting::SaveSettings()
{
	MouseSensitivity = ClampSensitivity(MouseSensitivity);
	UGameplayStatics::SaveGameToSlot(this, SaveSlotName, 0);
}

void UFPSUserSetting::SetMouseSensitivity(float NewSensitivity)
{
	MouseSensitivity = ClampSensitivity(NewSensitivity);
	ApplyMouseSensitivity();
	SaveSettings();
}

void UFPSUserSetting::ApplyMouseSensitivity()
{
	UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
	if (!InputSettings)
	{
		return;
	}

	auto ApplyAxisSensitivity = [InputSettings, this](const FName& AxisName)
	{
		/*FInputAxisProperties AxisProperties;
		if (InputSettings->GetAxisProperties(AxisName, AxisProperties))
		{
			AxisProperties.Sensitivity = MouseSensitivity;
			InputSettings->SetAxisProperties(AxisName, AxisProperties);
		}*/
	};

	ApplyAxisSensitivity(FName(TEXT("MouseX")));
	ApplyAxisSensitivity(FName(TEXT("MouseY")));
}

void UFPSUserSetting::LoadOnStartup()
{
	LoadOrCreateSettings();
}

float UFPSUserSetting::ClampSensitivity(const float Sensitivity)
{
	return FMath::Clamp(Sensitivity, MinMouseSensitivity, MaxMouseSensitivity);
}
