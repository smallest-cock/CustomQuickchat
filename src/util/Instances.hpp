#pragma once
#include "pch.h"

static constexpr int32_t INSTANCES_INTERATE_OFFSET = 10;

using GNames_t   = TArray<FNameEntry *> *;
using GObjects_t = TArray<UObject *> *;

class InstancesComponent {
public:
	InstancesComponent();
	~InstancesComponent();

public:
	void onCreate();
	void onDestroy();

	bool initGlobals(); // initialize globals for RLSDK

private:
	uintptr_t findGNamesAddress();
	uintptr_t findGMallocAddress();

	bool areGObjectsValid();
	bool areGNamesValid();
	bool checkGlobals();

private:
	std::map<std::string, class UClass *>    m_staticClasses;
	std::map<std::string, class UFunction *> m_staticFunctions;
	std::vector<class UObject *>             m_createdObjects;

	bool checkNotInName(UObject *obj, const std::string &str) { return obj->GetFullName().find(str) == std::string::npos; }

public:
	// Get the default constructor of a class type. Example: UGameData_TA* gameData = GetDefaultInstanceOf<UGameData_TA>();
	template <UObjectOrDerived T>
	T *getDefaultInstanceOf() {
		for (int32_t i = 0; i < (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); ++i) {
			UObject *uObject = UObject::GObjObjects()->at(i);
			if (!uObject || !uObject->IsA<T>())
				continue;
			if (uObject->ObjectFlags & RF_ClassDefaultObject)
				return reinterpret_cast<T *>(uObject);
		}
		return nullptr;
	}

	// Get the most current/active instance of a class. Example: UEngine* engine = GetInstanceOf<UEngine>();
	template <UObjectOrDerived T>
	T *getInstanceOf(bool omitDefaultsAndArchetypes = true) {
		for (int32_t i = (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i > 0; --i) {
			UObject *uObject = UObject::GObjObjects()->at(i);
			if (!validUObject(uObject) || !uObject->IsA<T>())
				continue;

			if (omitDefaultsAndArchetypes && uObject->ObjectFlags & RF_DefaultOrArchetypeFlags)
				continue;

			return static_cast<T *>(uObject);
		}

		return nullptr;
	}

	// Get all active instances of a class type. Example: std::vector<APawn*> pawns = GetAllInstancesOf<APawn>();
	template <UObjectOrDerived T>
	std::vector<T *> getAllInstancesOf(bool omitDefaultsAndArchetypes = true) {
		std::vector<T *> objectInstances;

		for (int32_t i = (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i > 0; --i) {
			UObject *uObject = UObject::GObjObjects()->at(i);
			if (!validUObject(uObject) || !uObject->IsA<T>())
				continue;

			if (omitDefaultsAndArchetypes && uObject->ObjectFlags & RF_DefaultOrArchetypeFlags)
				continue;

			objectInstances.push_back(static_cast<T *>(uObject));
		}

		return objectInstances;
	}

	// Get the most current/active instance of a class, if one isn't found it creates a new instance. Example: UEngine* engine =
	// GetInstanceOf<UEngine>();
	template <UObjectOrDerived T>
	T *getOrCreateInstance() {
		for (int32_t i = (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i > 0; --i) {
			UObject *uObject = UObject::GObjObjects()->at(i);
			if (!validUObject(uObject) || !uObject->IsA<T>())
				continue;

			if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags)
				continue;

			return static_cast<T *>(uObject);
		}

		return createInstance<T>();
	}

	// Get all active instances of a class type. Example: std::vector<APawn*> pawns = GetAllInstancesOf<APawn>();
	template <UObjectOrDerived T>
	std::vector<T *> getAllArchetypeInstancesOf() {
		std::vector<T *> objectInstances;

		for (int32_t i = (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i > 0; --i) {
			UObject *uObject = UObject::GObjObjects()->at(i);
			if (!validUObject(uObject) || !uObject->IsA<T>())
				continue;

			if (!(uObject->ObjectFlags & RF_ArchetypeObject))
				continue;

			objectInstances.push_back(static_cast<T *>(uObject));
		}

		return objectInstances;
	}

	// Get all default instances of a class type.
	template <typename T>
	std::vector<T *> getAllDefaultInstancesOf() {
		std::vector<T *> objectInstances;

		if (std::is_base_of<UObject, T>::value) {
			for (int32_t i = (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i > 0; i--) {
				UObject *uObject = UObject::GObjObjects()->at(i);

				if (uObject && uObject->IsA<T>()) {
					if (uObject->GetFullName().find("Default__") != std::string::npos) {
						objectInstances.push_back(static_cast<T *>(uObject));
					}
				}
			}
		}

		return objectInstances;
	}

	// Get an object instance by it's name and class type. Example: UTexture2D* texture = FindObject<UTexture2D>("WhiteSquare");
	template <typename T>
	T *findObject(const std::string &objectName, bool bStrictFind = false) {
		if (!std::is_base_of<UObject, T>::value)
			return nullptr;

		for (int32_t i = (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i > 0; --i) {
			UObject *uObject = UObject::GObjObjects()->at(i);
			if (!uObject || !uObject->IsA<T>())
				continue;

			std::string objectFullName = uObject->GetFullName();

			if (bStrictFind) {
				if (objectFullName == objectName)
					return static_cast<T *>(uObject);
			} else if (objectFullName.find(objectName) != std::string::npos)
				return static_cast<T *>(uObject);
		}

		return nullptr;
	}

	// Get all object instances by it's name and class type. Example: std::vector<UTexture2D*> textures =
	// FindAllObjects<UTexture2D>("Noise");
	template <typename T>
	std::vector<T *> findAllObjects(const std::string &objectName) {
		std::vector<T *> objectInstances;

		if (std::is_base_of<UObject, T>::value) {
			for (int32_t i = (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i > 0; i--) {
				UObject *uObject = UObject::GObjObjects()->at(i);

				if (uObject && uObject->IsA<T>()) {
					if (uObject->GetFullName().find(objectName) != std::string::npos) {
						objectInstances.push_back(static_cast<T *>(uObject));
					}
				}
			}
		}

		return objectInstances;
	}

	// ============================================ CUSTOM SHIT ============================================

	template <typename T>
	std::vector<T *> getFilteredPawns() {
		std::vector<T *> returnValues;

		AWorldInfo *Object = IAWorldInfo();

		if (std::is_base_of<APawn, T>::value) {
			if (Object && Object->PawnList) {
				for (APawn *pawn = Object->PawnList; pawn; pawn = pawn->NextPawn) {
					if (pawn && pawn->IsA<T>()) {
						returnValues.push_back(reinterpret_cast<T *>(pawn));
					}
				}
			}
		}

		return returnValues;
	}

	// =====================================================================================================

	class UClass    *findStaticClass(const std::string &className);
	class UFunction *findStaticFunction(const std::string &functionName);

	// Creates a new transient instance of a class which then adds it to globals.
	// YOU are required to make sure these objects eventually get eaten up by the garbage collector in some shape or form.
	// Example: UObject* newObject = CreateInstance<UObject>();
	template <UObjectOrDerived T>
	T *createInstance() {
		T *defaultObj = getDefaultInstanceOf<T>();
		if (!defaultObj)
			return nullptr;
		UClass *staticClass = T::StaticClass();
		if (!staticClass)
			return nullptr;

		T *createdObj = reinterpret_cast<T *>(defaultObj->DuplicateObject(defaultObj, defaultObj->Outer, staticClass));
		if (createdObj)
			markInvincible(createdObj);

		return createdObj;
	}

	// Set an object's flags to prevent it from being destoryed.
	void markInvincible(class UObject *object);

	// Set object as a temporary object and marks it for the garbage collector to destroy.
	void markForDestroy(class UObject *object);

private:
	class UCanvas             *I_UCanvas;
	class AHUD                *I_AHUD;
	class UGameViewportClient *I_UGameViewportClient;
	class APlayerController   *I_APlayerController;

public: // Use these functions to access these specific class instances, they will be set automatically; always remember to null check!
	class UEngine             *IUEngine();
	class UAudioDevice        *IUAudioDevice();
	class AWorldInfo          *IAWorldInfo();
	class UCanvas             *IUCanvas();
	class AHUD                *IAHUD();
	class UGameViewportClient *IUGameViewportClient();
	class ULocalPlayer        *IULocalPlayer();
	class APlayerController   *IAPlayerController();
	class UFileSystem         *IUFileSystem();
	struct FUniqueNetId        GetUniqueID();

public:
	AGFxHUD_TA      *hud          = nullptr;
	UGFxDataStore_X *dataStore    = nullptr;
	USaveData_TA    *saveData     = nullptr;
	UOnlinePlayer_X *onlinePlayer = nullptr;

public:
	AGFxHUD_TA        *getHUD();
	AGameEvent_TA     *getGameEvent();
	UGFxDataStore_X   *getDataStore();
	USaveData_TA      *getSaveData();
	UOnlinePlayer_X   *getOnlinePlayer();
	APlayerController *getPlayerController();
	AActor            *getCarActor();
	UProfile_TA       *getUserProfile();

public:
	template <UObjectOrDerived T>
	T *createOuter(UObject *outer) {
		auto *scriptORS = getInstanceOf<UScriptGroup_ORS>();
		if (!scriptORS)
			return nullptr;
		T      *objPtr    = nullptr;
		UClass *staticCls = T::StaticClass();
		if (staticCls)
			objPtr = reinterpret_cast<T *>(scriptORS->CreateObject(staticCls, outer));
		return objPtr;
	}

	template <UObjectOrDerived T>
	T *create() {
		return createOuter<T>(T::StaticClass()->Outer);
	}

	template <UObjectOrDerived T>
	T *create(T *obj) {
		if (!validUObject(obj))
			return create<T>();
		return createOuter<T>(obj->Outer);
	}

	// loads a UObject from a given package name
	template <UObjectOrDerived T>
	static T *loadObject(const std::string &packageName) {
		FString  packageNameFstr = FString::create(packageName);
		UObject *obj             = UObject::DynamicLoadObject(packageNameFstr, T::StaticClass(), true);
		if (!obj) {
			obj = UObject::LoadSeekFreeObject(T::StaticClass(), packageNameFstr);
			if (!obj)
				return nullptr;
		}

		return reinterpret_cast<T *>(obj);
	}

	void spawnNotification(const std::string &title, const std::string &content, int duration, bool log = false);
};

extern class InstancesComponent Instances;
