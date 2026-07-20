#pragma once

#include "CoreMinimal.h"

class AActor;
class UActorComponent;

/**
 * Reflection bridge to the project's Blueprint vital components
 * (BPC_health, BPC_stamina, BPC_LevelAndXP). These live in Content and cannot
 * be referenced statically from C++, so lookups, calls and delegate bindings
 * are performed by name with signature verification.
 */
struct FVitalsReflection
{
	/** Finds a component whose (generated) class name starts with the given prefix, e.g. "BPC_health". */
	static UActorComponent* FindComponentByClassNamePrefix(const AActor* Owner, const TCHAR* ClassNamePrefix);

	/**
	 * Calls a UFunction by name, filling the first numeric (double/float) input
	 * parameter with Value and any bool input parameter with bBoolParams.
	 * Returns false if the function does not exist.
	 */
	static bool CallFunctionWithDouble(UObject* Object, FName FunctionName, double Value, bool bBoolParams = true);

	static bool GetNumericProperty(const UObject* Object, FName PropertyName, double& OutValue);
	static bool SetNumericProperty(UObject* Object, FName PropertyName, double Value);

	/**
	 * Binds Target->HandlerName to a dynamic multicast delegate property on Source.
	 * The bind is skipped (returns false) when the handler signature does not
	 * exactly match the delegate signature, so a Blueprint-side change can never crash.
	 */
	static bool BindDynamicDelegate(UObject* Source, FName DelegateName, UObject* Target, FName HandlerName);
};
