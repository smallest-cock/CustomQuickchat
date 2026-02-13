#pragma once
#include <memory>
#include <unordered_set>

enum class HookType
{
	Pre,
	Post
};

struct HookKey
{
	std::string funcName;
	HookType    type;

	bool operator==(const HookKey &other) const { return funcName == other.funcName && type == other.type; }
};

namespace std
{
	template <>
	struct hash<HookKey>
	{
		size_t operator()(const HookKey &k) const { return hash<std::string>()(k.funcName) ^ (hash<int>()(static_cast<int>(k.type)) << 1); }
	};
}

class HookManager
{
	using CallbackWithCaller = std::function<void(ActorWrapper, void *, std::string)>;
	using CallbackNoCaller   = std::function<void(std::string)>;

private:
	std::shared_ptr<GameWrapper> m_gameWrapper;

	std::unordered_set<HookKey>                                  m_registeredEvents;
	std::unordered_map<HookKey, std::vector<CallbackWithCaller>> m_callbacksWithCaller;
	std::unordered_map<HookKey, std::vector<CallbackNoCaller>>   m_callbacks;

public:
	void init(const std::shared_ptr<GameWrapper> &gw) { m_gameWrapper = gw; }
	void registerHookPre(std::string event, CallbackWithCaller callback);
	void registerHookPre(std::string event, CallbackNoCaller callback);
	void registerHookPost(std::string event, CallbackWithCaller callback);
	void registerHookPost(std::string event, CallbackNoCaller callback);
	void commitHooks();
	void clearRegisteredEvents();

private:
	void commitHook(const std::pair<HookKey, std::vector<CallbackNoCaller>> &pair);
	void commitHook(const std::pair<HookKey, std::vector<CallbackWithCaller>> &pair);
};

extern HookManager g_hookManager;
