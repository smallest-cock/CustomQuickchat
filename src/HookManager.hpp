#pragma once
#include "bakkesmod/wrappers/Engine/ActorWrapper.h"
#include "bakkesmod/wrappers/GameWrapper.h"
#include <unordered_set>

enum class HookType
{
	Pre,
	Post
};

struct HookKey
{
	std::string functionName;
	HookType    type;

	bool operator==(const HookKey& other) const { return functionName == other.functionName && type == other.type; }
};

namespace std
{
template <> struct hash<HookKey>
{
	size_t operator()(const HookKey& k) const { return hash<std::string>()(k.functionName) ^ (hash<int>()(static_cast<int>(k.type)) << 1); }
};
} // namespace std

class HookManager
{
	std::shared_ptr<GameWrapper> m_gameWrapper;
	std::unordered_set<HookKey>  m_hookedEvents;

public:
	void init(const std::shared_ptr<GameWrapper>& gw) { m_gameWrapper = gw; }

	void hookEvent(const std::string& eventName, HookType type, const std::function<void(std::string)>& callback)
	{
		HookKey key{eventName, type};

		if (m_hookedEvents.contains(key))
		{
#ifdef THROW_ON_DOUBLE_HOOK // if this throws in the onLoad function BM will just catch the exception and log it in console. aka no crash :(
			throw std::runtime_error("Already hooked " + std::string(type == HookType::Pre ? "Pre" : "Post") + ": \"" + eventName + "\"");
#else
			LOGERROR("Already hooked {}: \"{}\"", type == HookType::Pre ? "Pre" : "Post", eventName);
#endif
			return;
		}

		switch (type)
		{
		case HookType::Pre:
			m_gameWrapper->HookEvent(eventName, callback);
			break;
		case HookType::Post:
			m_gameWrapper->HookEventPost(eventName, callback);
			break;
		default:
			LOGERROR("Invalid hook type for event: \"{}\"", eventName);
			break;
		}

		m_hookedEvents.insert(key);
		LOG("Hooked function {}: \"{}\"", type == HookType::Pre ? "PRE" : "POST", eventName);
	}

	void hookEvent(const std::string& eventName, HookType type, const std::function<void(ActorWrapper, void*, std::string)>& callback)
	{
		HookKey key{eventName, type};

		if (m_hookedEvents.contains(key))
		{
			LOGERROR("Already hooked {}: \"{}\"", type == HookType::Pre ? "Pre" : "Post", eventName);
			return;
		}

		switch (type)
		{
		case HookType::Pre:
			m_gameWrapper->HookEventWithCaller<ActorWrapper>(eventName, callback);
			break;
		case HookType::Post:
			m_gameWrapper->HookEventWithCallerPost<ActorWrapper>(eventName, callback);
			break;
		default:
			LOGERROR("Invalid hook type for event: \"{}\"", eventName);
			break;
		}

		m_hookedEvents.insert(key);
		LOG("Hooked function {} (with caller): \"{}\"", type == HookType::Pre ? "PRE" : "POST", eventName);
	}

	void unhookEvent(const std::string& eventName, HookType type)
	{
		HookKey key{eventName, type};

		auto it = m_hookedEvents.find(key);
		if (it == m_hookedEvents.end())
		{
			LOGERROR(
			    "Unhooking function {} was unsuccessful (it was never hooked): \"{}\"", type == HookType::Pre ? "Pre" : "Post", eventName);
			return;
		}

		switch (type)
		{
		case HookType::Pre:
			m_gameWrapper->UnhookEvent(eventName);
			break;
		case HookType::Post:
			m_gameWrapper->UnhookEventPost(eventName);
			break;
		default:
			LOGERROR("Invalid hook type for event: \"{}\"", eventName);
			break;
		}

		m_hookedEvents.erase(it);
		LOG("Unhooked function {}: \"{}\"", type == HookType::Pre ? "Pre" : "Post", eventName);
	}

	void unhookAllEvents()
	{
		for (const auto& key : m_hookedEvents)
		{
			switch (key.type)
			{
			case HookType::Pre:
				m_gameWrapper->UnhookEvent(key.functionName);
				break;
			case HookType::Post:
				m_gameWrapper->UnhookEventPost(key.functionName);
				break;
			default:
				LOG("ERROR: Invalid hook type for event: \"{}\"", key.functionName);
				break;
			}
		}

		LOG("Unhooked all events ({} total)", m_hookedEvents.size());
		m_hookedEvents.clear();
	}
};

extern class HookManager Hooks;
