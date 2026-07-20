#include "VitalsReflection.h"

#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

UActorComponent* FVitalsReflection::FindComponentByClassNamePrefix(const AActor* Owner, const TCHAR* ClassNamePrefix)
{
	if (!Owner)
	{
		return nullptr;
	}

	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (!Component)
		{
			continue;
		}
		for (const UClass* Class = Component->GetClass(); Class; Class = Class->GetSuperClass())
		{
			if (Class->GetName().StartsWith(ClassNamePrefix))
			{
				return Component;
			}
		}
	}
	return nullptr;
}

bool FVitalsReflection::CallFunctionWithDouble(UObject* Object, const FName FunctionName, const double Value, const bool bBoolParams)
{
	if (!Object)
	{
		return false;
	}

	UFunction* Function = Object->FindFunction(FunctionName);
	if (!Function)
	{
		return false;
	}

	// Build the parameter block dynamically so Blueprint signature changes
	// (e.g. float vs double, extra outputs) never corrupt memory.
	TArray<uint8> Params;
	Params.SetNumZeroed(Function->ParmsSize);

	bool bValueAssigned = false;
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		FProperty* Prop = *It;
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
		{
			continue;
		}
		if (!bValueAssigned)
		{
			if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
			{
				DoubleProp->SetPropertyValue_InContainer(Params.GetData(), Value);
				bValueAssigned = true;
				continue;
			}
			if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
			{
				FloatProp->SetPropertyValue_InContainer(Params.GetData(), static_cast<float>(Value));
				bValueAssigned = true;
				continue;
			}
			if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
			{
				IntProp->SetPropertyValue_InContainer(Params.GetData(), FMath::RoundToInt32(Value));
				bValueAssigned = true;
				continue;
			}
		}
		if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
		{
			BoolProp->SetPropertyValue_InContainer(Params.GetData(), bBoolParams);
		}
	}

	Object->ProcessEvent(Function, Params.GetData());

	// Destroy any non-trivial parameters that ProcessEvent may have constructed.
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Params.GetData());
	}
	return true;
}

bool FVitalsReflection::GetNumericProperty(const UObject* Object, const FName PropertyName, double& OutValue)
{
	if (!Object)
	{
		return false;
	}
	const FProperty* Prop = Object->GetClass()->FindPropertyByName(PropertyName);
	if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
	{
		OutValue = DoubleProp->GetPropertyValue_InContainer(Object);
		return true;
	}
	if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
	{
		OutValue = FloatProp->GetPropertyValue_InContainer(Object);
		return true;
	}
	if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
	{
		OutValue = IntProp->GetPropertyValue_InContainer(Object);
		return true;
	}
	return false;
}

bool FVitalsReflection::SetNumericProperty(UObject* Object, const FName PropertyName, const double Value)
{
	if (!Object)
	{
		return false;
	}
	const FProperty* Prop = Object->GetClass()->FindPropertyByName(PropertyName);
	if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
	{
		DoubleProp->SetPropertyValue_InContainer(Object, Value);
		return true;
	}
	if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
	{
		FloatProp->SetPropertyValue_InContainer(Object, static_cast<float>(Value));
		return true;
	}
	if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
	{
		IntProp->SetPropertyValue_InContainer(Object, FMath::RoundToInt32(Value));
		return true;
	}
	return false;
}

bool FVitalsReflection::BindDynamicDelegate(UObject* Source, const FName DelegateName, UObject* Target, const FName HandlerName)
{
	if (!Source || !Target)
	{
		return false;
	}

	const FMulticastDelegateProperty* DelegateProp =
		CastField<FMulticastDelegateProperty>(Source->GetClass()->FindPropertyByName(DelegateName));
	if (!DelegateProp || !DelegateProp->SignatureFunction)
	{
		return false;
	}

	const UFunction* Handler = Target->FindFunction(HandlerName);
	if (!Handler || !Handler->IsSignatureCompatibleWith(DelegateProp->SignatureFunction))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("VitalsReflection: cannot bind %s.%s -> %s.%s (missing or incompatible signature)"),
			*Source->GetClass()->GetName(), *DelegateName.ToString(),
			*Target->GetClass()->GetName(), *HandlerName.ToString());
		return false;
	}

	FScriptDelegate Delegate;
	Delegate.BindUFunction(Target, HandlerName);
	DelegateProp->AddDelegate(MoveTemp(Delegate), Source);
	return true;
}
