#include "INIReader.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/Log.h"
#include "component.h"
#include "system.h"
Log_SetChannel(SystemConfig);

static std::unique_ptr<System> CreateSystem(INIReader& ini, Error* error)
{
  // Retrieve the system type name, this determines which class we need to create.
  std::string system_type = ini.Get("System", "Type", "");
  Log_InfoPrintf("System class: '%s'", system_type.c_str());
  if (system_type.empty())
  {
    error->SetErrorUserFormatted(4, "System type not specified.");
    return nullptr;
  }

  // Create an instance of this type.
  const ObjectTypeInfo* system_type_info = ObjectTypeInfo::GetRegistry().GetTypeInfoByName(system_type.c_str());
  if (!system_type_info)
  {
    error->SetErrorUserFormatted(5, "Unknown system type '%s'", system_type.c_str());
    return nullptr;
  }
  else if (!system_type_info->CanCreateInstance())
  {
    error->SetErrorUserFormatted(6, "Type '%s' cannot be created directly", system_type_info->GetTypeName());
    return nullptr;
  }
  else if (!system_type_info->IsDerived(OBJECT_TYPEINFO(System)))
  {
    error->SetErrorUserFormatted(7, "Type '%s' is not a system type", system_type_info->GetTypeName());
    return nullptr;
  }

  Object* obj = system_type_info->CreateInstance();
  Assert(obj && obj->IsDerived<System>());
  return std::unique_ptr<System>(obj->Cast<System>());
}

static Component* CreateComponent(const std::string& component_identifier, const std::string& component_type,
                                  Error* error)
{
  // Create an instance of this type.
  const ObjectTypeInfo* type_info = ObjectTypeInfo::GetRegistry().GetTypeInfoByName(component_type.c_str());
  if (!type_info)
  {
    error->SetErrorUserFormatted(8, "Unknown component type '%s'", component_type.c_str());
    return nullptr;
  }
  else if (!type_info->CanCreateInstance())
  {
    error->SetErrorUserFormatted(9, "Type '%s' cannot be created directly", type_info->GetTypeName());
    return nullptr;
  }
  else if (!type_info->IsDerived(OBJECT_TYPEINFO(Component)))
  {
    error->SetErrorUserFormatted(10, "Type '%s' is not a component type", type_info->GetTypeName());
    return nullptr;
  }

  Log_InfoPrintf("Creating instance '%s' of optional component '%s'", component_identifier.c_str(),
                 type_info->GetTypeName());
  Object* obj = type_info->CreateInstance();
  Assert(obj && obj->IsDerived<Component>());
  return obj->Cast<Component>();
}

static bool ApplyProperties(Object* object, const std::string& object_name, INIReader& ini,
                            const std::set<std::string>& fields, Error* error)
{
  const ObjectTypeInfo* type_info = object->GetTypeInfo();
  for (const std::string& name : fields)
  {
    // Ignore type, since it's used when creating.
    if (name == "Type")
      continue;

    const PROPERTY_DECLARATION* prop = type_info->GetPropertyDeclarationByName(name.c_str());
    if (!prop)
    {
      error->SetErrorUserFormatted(13, "Failed to set property '%s' on '%s' (%s), property does not exist",
                                   name.c_str(), object_name.c_str(), type_info->GetTypeName());
      return false;
    }
    else if (prop->Flags & PROPERTY_FLAG_READ_ONLY)
    {
      error->SetErrorUserFormatted(14, "Failed to set property '%s' on '%s' (%s), property is read only", name.c_str(),
                                   object_name.c_str(), type_info->GetTypeName());
      return false;
    }

    const std::string& value = ini.Get(object_name, name, "");
    Log_DevPrintf("Setting property '%s' to '%s' on object '%s' (%s)", name.c_str(), value.c_str(), object_name.c_str(),
                  type_info->GetTypeName());

    if (!SetPropertyValueFromString(object, prop, value.c_str()))
    {
      error->SetErrorUserFormatted(15, "Failed to set property '%s' on '%s' (%s) to '%s'", name.c_str(),
                                   object_name.c_str(), type_info->GetTypeName(), value.c_str());
      return false;
    }
  }

  return true;
}

std::unique_ptr<System> System::ParseConfig(const char* filename, Error* error)
{
  INIReader ini(filename);
  if (ini.ParseError() != 0)
  {
    int res = ini.ParseError();
    if (res > 0)
      error->SetErrorUserFormatted(1, "INI parse error on line %d", error);
    else if (res == -1)
      error->SetErrorUserFormatted(2, "Failed to open INI file %s", filename);
    else
      error->SetErrorUserFormatted(3, "INI parse error");

    return nullptr;
  }

  std::unique_ptr<System> system = CreateSystem(ini, error);
  if (!system)
    return nullptr;

  // Parse properties for system.
  if (!ApplyProperties(system.get(), "System", ini, ini.GetFields("System"), error))
    return nullptr;

  // Apply component properties for both fixed and optional components.
  for (const std::string& component_identifier : ini.GetSections())
  {
    // Skip system, we already did it.
    if (component_identifier == "System")
      continue;

    // If it has a type field, then it's an optional component.
    Component* component = system->GetComponentByIdentifier(component_identifier.c_str());
    const std::string component_type = ini.Get(component_identifier, "Type", "");
    if (component && !component_type.empty())
    {
      error->SetErrorUserFormatted(11, "Component '%s' already exists in the system as a built-in",
                                   component_identifier.c_str());
      return nullptr;
    }
    else if (!component)
    {
      if (component_type.empty())
      {
        error->SetErrorUserFormatted(12, "Component '%s' is not built-in, and has no type specified.",
                                     component_identifier.c_str());
        return nullptr;
      }

      // Create the component.
      component = ::CreateComponent(component_identifier, component_type, error);
      if (!component)
        return nullptr;

      // Add the component to the system before setting it up.
      system->AddComponent(component);
    }

    // Apply properties to component.
    if (!ApplyProperties(component, component_identifier, ini, ini.GetFields(component_identifier), error))
      return nullptr;
  }

  return system;
}
