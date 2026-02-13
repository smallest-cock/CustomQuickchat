#include "pch.h"
#include "Instances.hpp"
#include "Logging.hpp"
#include "ModUtils/util/Utils.hpp"
#include <libloaderapi.h>
#include <minwindef.h>

InstancesComponent::InstancesComponent() { onCreate(); }
InstancesComponent::~InstancesComponent() { onDestroy(); }

void InstancesComponent::onCreate() {
	I_UCanvas             = nullptr;
	I_AHUD                = nullptr;
	I_UGameViewportClient = nullptr;
	I_APlayerController   = nullptr;
}

void InstancesComponent::onDestroy() {
	m_staticClasses.clear();
	m_staticFunctions.clear();

	for (UObject *uObject : m_createdObjects) {
		if (!uObject)
			continue;

		markForDestroy(uObject);
	}

	m_createdObjects.clear();
}

// ========================================= init RLSDK globals ===========================================

constexpr auto MODULE_NAME = L"RocketLeague.exe";
HMODULE        rlModule    = GetModuleHandle(MODULE_NAME);

uintptr_t InstancesComponent::findGNamesAddress() {
	constexpr auto sig       = "?? ?? ?? ?? ?? ?? 00 00 ?? ?? 01 00 35 25 02 00";
	uintptr_t      foundAddr = Memory::findPattern(rlModule, sig);
	if (!foundAddr) {
		LOGERROR("GNames wasn't found! Returning 0...");
		return 0;
	}
	return foundAddr;
}

uintptr_t InstancesComponent::findGMallocAddress() {
	constexpr auto sig       = "48 89 0D ?? ?? ?? ?? 48 8B 01 FF 50 60";
	uintptr_t      foundAddr = Memory::findPattern(rlModule, sig);
	if (!foundAddr) {
		LOGERROR("GMalloc wasn't found! Returning 0...");
		return 0;
	}
	return Memory::getRipRelativeAddr(foundAddr, 3);
}

bool InstancesComponent::initGlobals() {
	uintptr_t gnamesAddr = findGNamesAddress();
	if (!gnamesAddr) {
		LOGERROR("Failed to find GNames address via pattern scan");
		return false;
	}
	GNames   = reinterpret_cast<GNames_t>(gnamesAddr);
	GObjects = reinterpret_cast<GObjects_t>(gnamesAddr + 0x48);

	uintptr_t gmallocAddr = findGMallocAddress();
	if (!gmallocAddr) {
		LOGERROR("Failed to find GMalloc address via pattern scan");
		return false;
	}
	GMalloc = gmallocAddr;

	return checkGlobals();
}

bool InstancesComponent::areGObjectsValid() {
	if (UObject::GObjObjects()->size() > 0 && UObject::GObjObjects()->capacity() > UObject::GObjObjects()->size()) {
		if (UObject::GObjObjects()->at(0)->GetFullName() == "Class Core.Config_ORS")
			return true;
	}
	return false;
}

bool InstancesComponent::areGNamesValid() {
	if (FName::Names()->size() > 0 && FName::Names()->capacity() > FName::Names()->size()) {
		if (FName(0).ToString() == "None")
			return true;
	}
	return false;
}

bool InstancesComponent::checkGlobals() {
	bool gnamesValid   = GNames && areGNamesValid();
	bool gobjectsValid = GObjects && areGObjectsValid();
	if (!gnamesValid || !gobjectsValid) {
		LOG("(onLoad) Error: RLSDK classes are wrong... plugin needs an update :(");
		LOG(std::format("GNames valid: {} -- GObjects valid: {}", gnamesValid, gobjectsValid));
		return false;
	}

	LOG("Globals Initialized :)");
	return true;
}

// ===========================================================================================================

class UClass *InstancesComponent::findStaticClass(const std::string &className) {
	if (m_staticClasses.empty()) {
		for (int32_t i = 0; i < (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i++) {
			UObject *uObject = UObject::GObjObjects()->at(i);

			if (uObject) {
				if ((uObject->GetFullName().find("Class") == 0)) {
					m_staticClasses[uObject->GetFullName()] = static_cast<UClass *>(uObject);
				}
			}
		}
	}

	if (m_staticClasses.contains(className)) {
		return m_staticClasses[className];
	}

	return nullptr;
}

class UFunction *InstancesComponent::findStaticFunction(const std::string &className) {
	if (m_staticFunctions.empty()) {
		for (int32_t i = 0; i < (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i++) {
			UObject *uObject = UObject::GObjObjects()->at(i);

			if (uObject) {
				if (uObject && uObject->IsA<UFunction>()) {
					m_staticFunctions[uObject->GetFullName()] = static_cast<UFunction *>(uObject);
				}
			}
		}
	}

	if (m_staticFunctions.contains(className)) {
		return m_staticFunctions[className];
	}

	return nullptr;
}

void InstancesComponent::markInvincible(class UObject *object) {
	if (!object)
		return;

	object->ObjectFlags &= ~EObjectFlags::RF_Transient;
	object->ObjectFlags &= ~EObjectFlags::RF_TagGarbage;
	object->ObjectFlags &= ~EObjectFlags::RF_PendingKill;
	object->ObjectFlags |= EObjectFlags::RF_DisregardForGC;
	object->ObjectFlags |= EObjectFlags::RF_RootSet;
}

void InstancesComponent::markForDestroy(class UObject *object) {
	if (!object)
		return;

	object->ObjectFlags |= EObjectFlags::RF_Transient;
	object->ObjectFlags |= EObjectFlags::RF_TagGarbage;
	object->ObjectFlags |= EObjectFlags::RF_PendingKill;

	auto objectIt = std::find(m_createdObjects.begin(), m_createdObjects.end(), object);
	if (objectIt != m_createdObjects.end())
		m_createdObjects.erase(objectIt);
}

class UEngine             *InstancesComponent::IUEngine() { return UEngine::GetEngine(); }
class UAudioDevice        *InstancesComponent::IUAudioDevice() { return UEngine::GetAudioDevice(); }
class AWorldInfo          *InstancesComponent::IAWorldInfo() { return UEngine::GetCurrentWorldInfo(); }
class UCanvas             *InstancesComponent::IUCanvas() { return I_UCanvas; }
class AHUD                *InstancesComponent::IAHUD() { return I_AHUD; }
class UFileSystem         *InstancesComponent::IUFileSystem() { return (UFileSystem *)UFileSystem::StaticClass(); }
class UGameViewportClient *InstancesComponent::IUGameViewportClient() { return I_UGameViewportClient; }

class ULocalPlayer *InstancesComponent::IULocalPlayer() {
	UEngine *engine = IUEngine();

	if (engine && engine->GamePlayers[0]) {
		return engine->GamePlayers[0];
	}

	return nullptr;
}

class APlayerController *InstancesComponent::IAPlayerController() {
	if (!I_APlayerController) {
		I_APlayerController = getInstanceOf<APlayerController>();
	}

	return I_APlayerController;
}

struct FUniqueNetId InstancesComponent::GetUniqueID() {
	ULocalPlayer *localPlayer = IULocalPlayer();

	if (localPlayer) {
		return localPlayer->eventGetUniqueNetId();
	}

	return FUniqueNetId{};
}

// ======================= get instance funcs =========================

AGFxHUD_TA *InstancesComponent::getHUD() {
	if (!hud || !hud->IsA<AGFxHUD_TA>()) {
		hud = getInstanceOf<AGFxHUD_TA>();
	}

	return hud;
}

AGameEvent_TA *InstancesComponent::getGameEvent() {
	AGFxHUD_TA *hud = getHUD();
	if (!hud || !validUObject(hud->GameEvent))
		return nullptr;
	return hud->GameEvent;
}

UGFxDataStore_X *InstancesComponent::getDataStore() {
	if (!dataStore || !dataStore->IsA<UGFxDataStore_X>()) {
		dataStore = getInstanceOf<UGFxDataStore_X>();
	}

	return dataStore;
}

USaveData_TA *InstancesComponent::getSaveData() {
	if (!saveData || !saveData->IsA<USaveData_TA>()) {
		saveData = getInstanceOf<USaveData_TA>();
	}

	return saveData;
}

UOnlinePlayer_X *InstancesComponent::getOnlinePlayer() {
	if (!onlinePlayer || !onlinePlayer->IsA<UOnlinePlayer_X>()) {
		onlinePlayer = getInstanceOf<UOnlinePlayer_X>();
	}

	return onlinePlayer;
}

APlayerController *InstancesComponent::getPlayerController() {
	ULocalPlayer *lp = Instances.IULocalPlayer();
	if (!validUObject(lp) || !validUObject(lp->Actor) || !lp->Actor->IsA<APlayerController>())
		return nullptr;

	return static_cast<APlayerController *>(lp->Actor);
}

AActor *InstancesComponent::getCarActor() {
	auto *pc = Instances.getPlayerController();
	if (!pc)
		return nullptr;
	for (auto *child : pc->Children) {
		if (!child)
			continue;
		if (child->IsA<ACarPreviewActor_TA>() || child->IsA<ACar_TA>())
			return child;
	}
	return nullptr;
}

UProfile_TA *InstancesComponent::getUserProfile() {
	auto *lp = IULocalPlayer();
	if (!lp || !lp->IsA<ULocalPlayer_TA>())
		return nullptr;
	auto *lpta = static_cast<ULocalPlayer_TA *>(lp);
	return lpta->Profile;
}

// ====================================== misc funcs ================================================

void InstancesComponent::spawnNotification(const std::string &title, const std::string &content, int duration, bool log) {
	UNotificationManager_TA *notificationManager = Instances.getInstanceOf<UNotificationManager_TA>();
	if (!notificationManager)
		return;

	static UClass *notificationClass = nullptr;
	if (!notificationClass) {
		notificationClass = UGenericNotification_TA::StaticClass();
	}

	UNotification_TA *notification = notificationManager->PopUpOnlyNotification(notificationClass);
	if (!notification)
		return;

	FString titleFStr   = FString::create(title);
	FString contentFStr = FString::create(content);

	notification->SetTitle(titleFStr);
	notification->SetBody(contentFStr);
	notification->PopUpDuration = duration;

	if (log) {
		LOG("[{}] {}", title.c_str(), content.c_str());
	}
}

class InstancesComponent Instances{};
