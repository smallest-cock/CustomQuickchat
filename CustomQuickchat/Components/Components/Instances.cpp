#include "pch.h"
#include "Instances.hpp"


InstancesComponent::InstancesComponent() { OnCreate(); }

InstancesComponent::~InstancesComponent() { OnDestroy(); }

void InstancesComponent::OnCreate()
{
    I_UCanvas = nullptr;
    I_AHUD = nullptr;
    I_UGameViewportClient = nullptr;
    I_APlayerController = nullptr;
}

void InstancesComponent::OnDestroy()
{
    m_staticClasses.clear();
    m_staticFunctions.clear();

    for (UObject* uObject : m_createdObjects)
    {
        if (uObject)
        {
            MarkForDestroy(uObject);
        }
    }

    m_createdObjects.clear();
}


// ========================================= to initialize globals ===========================================

uintptr_t InstancesComponent::FindPattern(HMODULE module, const unsigned char* pattern, const char* mask)
{
    MODULEINFO info = { };
    GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

    uintptr_t start = reinterpret_cast<uintptr_t>(module);
    size_t length = info.SizeOfImage;

    size_t pos = 0;
    size_t maskLength = std::strlen(mask) - 1;

    for (uintptr_t retAddress = start; retAddress < start + length; retAddress++)
    {
        if (*reinterpret_cast<unsigned char*>(retAddress) == pattern[pos] || mask[pos] == '?')
        {
            if (pos == maskLength)
            {
                return (retAddress - maskLength);
            }
            pos++;
        }
        else
        {
            retAddress -= pos;
            pos = 0;
        }
    }
    return NULL;
}

uintptr_t InstancesComponent::GetGNamesAddress() 
{
    constexpr unsigned char GNamesPattern[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00";
    constexpr char GNamesMask[] = "??????xx??xxxxxx";

    uintptr_t GNamesAddress = FindPattern(GetModuleHandle(L"RocketLeague.exe"), GNamesPattern, GNamesMask);

    return GNamesAddress;
}

uintptr_t InstancesComponent::GetGObjectsAddress() 
{
    return GetGNamesAddress() + 0x48;
}

void InstancesComponent::InitGlobals()
{
    uintptr_t gnamesAddr = GetGNamesAddress();
    GNames = reinterpret_cast<TArray<FNameEntry*>*>(gnamesAddr);
    GObjects = reinterpret_cast<TArray<UObject*>*>(gnamesAddr + 0x48);
}

bool InstancesComponent::AreGObjectsValid()
{
    if (UObject::GObjObjects()->size() > 0 && UObject::GObjObjects()->capacity() > UObject::GObjObjects()->size())
    {
        if (UObject::GObjObjects()->at(0)->GetFullName() == "Class Core.Config_ORS")
        {
            return true;
        }
    }
    return false;
}

bool InstancesComponent::AreGNamesValid()
{
    if (FName::Names()->size() > 0 && FName::Names()->capacity() > FName::Names()->size())
    {
        if (FName(0).ToString() == "None")
        {
            return true;
        }
    }
    return false;
}

bool InstancesComponent::CheckGlobals()
{
    bool gnamesValid = GNames && AreGNamesValid();
    bool gobjectsValid = GObjects && AreGObjectsValid();
    if (!gnamesValid || !gobjectsValid)
    {
        LOG("(onLoad) Error: RLSDK classes are wrong... plugin needs an update :(");
        LOG(std::format("GNames valid: {} -- GObjects valid: {}", gnamesValid, gobjectsValid));
        return false;
    }

    LOG("Globals Initialized :)");
    return true;
}

// ===========================================================================================================


class UClass* InstancesComponent::FindStaticClass(const std::string& className)
{
    if (m_staticClasses.empty())
    {
        for (int32_t i = 0; i < (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i++)
        {
            UObject* uObject = UObject::GObjObjects()->at(i);

            if (uObject)
            {
                if ((uObject->GetFullName().find("Class") == 0))
                {
                    m_staticClasses[uObject->GetFullName()] = static_cast<UClass*>(uObject);
                }
            }
        }
    }

    if (m_staticClasses.contains(className))
    {
        return m_staticClasses[className];
    }

    return nullptr;
}

class UFunction* InstancesComponent::FindStaticFunction(const std::string& className)
{
    if (m_staticFunctions.empty())
    {
        for (int32_t i = 0; i < (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i++)
        {
            UObject* uObject = UObject::GObjObjects()->at(i);

            if (uObject)
            {
                if (uObject && uObject->IsA<UFunction>())
                {
                    m_staticFunctions[uObject->GetFullName()] = static_cast<UFunction*>(uObject);
                }
            }
        }
    }

    if (m_staticFunctions.contains(className))
    {
        return m_staticFunctions[className];
    }

    return nullptr;
}

void InstancesComponent::MarkInvincible(class UObject* object)
{
    if (object)
    {
        object->ObjectFlags &= ~EObjectFlags::RF_Transient;
        object->ObjectFlags &= ~EObjectFlags::RF_TagGarbage;
        object->ObjectFlags &= ~EObjectFlags::RF_PendingKill;
        object->ObjectFlags |= EObjectFlags::RF_DisregardForGC;
        object->ObjectFlags |= EObjectFlags::RF_RootSet;
    }
}

void InstancesComponent::MarkForDestroy(class UObject* object)
{
    if (object)
    {
        object->ObjectFlags |= EObjectFlags::RF_Transient;
        object->ObjectFlags |= EObjectFlags::RF_TagGarbage;
        object->ObjectFlags |= EObjectFlags::RF_PendingKill;

        auto objectIt = std::find(m_createdObjects.begin(), m_createdObjects.end(), object);

        if (objectIt != m_createdObjects.end())
        {
            m_createdObjects.erase(objectIt);
        }
    }
}

class UEngine* InstancesComponent::IUEngine()
{
    return UEngine::GetEngine();
}

class UAudioDevice* InstancesComponent::IUAudioDevice()
{
    return UEngine::GetAudioDevice();
}

class AWorldInfo* InstancesComponent::IAWorldInfo()
{
    return UEngine::GetCurrentWorldInfo();
}

class UCanvas* InstancesComponent::IUCanvas()
{
    return I_UCanvas;
}

class AHUD* InstancesComponent::IAHUD()
{
    return I_AHUD;
}

class UFileSystem* InstancesComponent::IUFileSystem()
{
    return (UFileSystem*)UFileSystem::StaticClass();
}

class UGameViewportClient* InstancesComponent::IUGameViewportClient()
{
    return I_UGameViewportClient;
}

class ULocalPlayer* InstancesComponent::IULocalPlayer()
{
    UEngine* engine = IUEngine();

    if (engine && engine->GamePlayers[0])
    {
        return engine->GamePlayers[0];
    }

    return nullptr;
}

class APlayerController* InstancesComponent::IAPlayerController()
{
    return I_APlayerController;
}

struct FUniqueNetId InstancesComponent::GetUniqueID()
{
    ULocalPlayer* localPlayer = IULocalPlayer();

    if (localPlayer)
    {
        return localPlayer->eventGetUniqueNetId();
    }

    return FUniqueNetId{};
}


// ======================= get instance funcs =========================

AGFxHUD_TA* InstancesComponent::GetHUD()
{
    if (hud)
        return hud;

    return GetInstanceOf<AGFxHUD_TA>();
}

UGFxDataStore_X* InstancesComponent::GetDataStore()
{
    if (dataStore)
        return dataStore;

    return GetInstanceOf<UGFxDataStore_X>();
}

UOnlinePlayer_X* InstancesComponent::GetOnlinePlayer()
{
    if (onlinePlayer)
        return onlinePlayer;

    return GetInstanceOf<UOnlinePlayer_X>();
}


// ====================================== misc funcs ================================================

void InstancesComponent::SpawnNotification(const std::string& title, const std::string& content, int duration, bool log)
{
    UNotificationManager_TA* notificationManager = Instances.GetInstanceOf<UNotificationManager_TA>();
    if (!notificationManager)
        return;

    if (!notificationClass)
    {
        notificationClass = UGenericNotification_TA::StaticClass();
    }

    UNotification_TA* notification = notificationManager->PopUpOnlyNotification(notificationClass);
    if (!notification)
        return;

    FString titleFStr = StringUtils::newFString(title);
    FString contentFStr = StringUtils::newFString(content);

    notification->SetTitle(titleFStr);
    notification->SetBody(contentFStr);
    notification->PopUpDuration = duration;

    if (log)
    {
        LOG("[{}] {}", title.c_str(), content.c_str());
    }
}


void InstancesComponent::SendChat(const std::string& chat, EChatChannel chatMode, bool log)
{
    UGFxData_Chat_TA* chatBox = GetInstanceOf<UGFxData_Chat_TA>();
    if (!chatBox) {
        LOG("UGFxData_Chat_TA* is null!");
        return;
    }

    FString chatFStr = StringUtils::newFString(chat);

    if (chatMode == EChatChannel::EChatChannel_Match)
    {
        chatBox->SendChatMessage(chatFStr, 0);      // match (lobby) chat

        if (log)
        {
            LOG("Sent chat: '{}'", chat);
        }
    }
    else if (chatMode == EChatChannel::EChatChannel_Team)
    {
        chatBox->SendTeamChatMessage(chatFStr, 0);  // team chat

        if (log)
        {
            LOG("Sent chat: [Team] '{}'", chat);
        }
    }
    else if (chatMode == EChatChannel::EChatChannel_Party)
    {
        chatBox->SendPartyChatMessage(chatFStr, 0); // party chat

        if (log)
        {
            LOG("Sent chat: [Party] '{}'", chat);
        }
    }
}


void InstancesComponent::SetChatTimeoutMsg(const std::string& newMsg, AGFxHUD_TA* hud)
{
    if (!hud)
    {
        hud = GetInstanceOf<AGFxHUD_TA>();
        if (!hud)
            return;
    }

    if (hud->ChatDisabledMessage.ToString() == newMsg)
        return;
     
    hud->ChatDisabledMessage = StringUtils::newFString(newMsg);    // overwrite ChatDisabledMessage
    LOG("Set chat timeout message: \"{}\"", Format::EscapeBraces(newMsg));
}



class InstancesComponent Instances {};