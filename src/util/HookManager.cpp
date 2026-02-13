#include "pch.h"
#include "HookManager.hpp"
#include "bakkesmod/wrappers/Engine/ActorWrapper.h"
#include "util/Logging.hpp"

void HookManager::registerHookPre(std::string event, CallbackWithCaller callback) {
	HookKey key{event, HookType::Pre};
	m_callbacksWithCaller[key].emplace_back(std::move(callback));
	m_registeredEvents.insert(key);
	LOG("Registered PRE hook with caller: {}", event);
}

void HookManager::registerHookPre(std::string event, CallbackNoCaller callback) {
	HookKey key{event, HookType::Pre};
	m_callbacks[key].emplace_back(std::move(callback));
	m_registeredEvents.insert(key);
	LOG("Registered PRE hook: {}", event);
}

void HookManager::registerHookPost(std::string event, CallbackWithCaller callback) {
	HookKey key{event, HookType::Post};
	m_callbacksWithCaller[key].emplace_back(std::move(callback));
	m_registeredEvents.insert(key);
	LOG("Registered POST hook with caller: {}", event);
}

void HookManager::registerHookPost(std::string event, CallbackNoCaller callback) {
	HookKey key{event, HookType::Post};
	m_callbacks[key].emplace_back(std::move(callback));
	m_registeredEvents.insert(key);
	LOG("Registered POST hook for event: {}", event);
}

void HookManager::commitHooks() {
	if (!m_gameWrapper) {
		LOGERROR("GameWrapper not initialized!");
		return;
	}

	LOG("Committing {} registered hook events", m_registeredEvents.size());

	// Process hooks that have both types registered (combined)
	for (const auto &key : m_registeredEvents) {
		auto noCallerIt   = m_callbacks.find(key);
		auto withCallerIt = m_callbacksWithCaller.find(key);

		if (noCallerIt != m_callbacks.end() && withCallerIt != m_callbacksWithCaller.end()) {
			LOG("Creating combined hook for: {} ({} callbacks total)",
			    key.funcName,
			    noCallerIt->second.size() + withCallerIt->second.size());

			// Create combined callback
			auto combinedCallback = [withCallerVec = std::move(withCallerIt->second), noCallerVec = std::move(noCallerIt->second)](
			                            ActorWrapper c, void *p, std::string e) mutable {
				// With-caller callbacks first
				for (const auto &func : withCallerVec) {
					if (func)
						func(c, p, e);
				}
				// Then no-caller callbacks
				for (const auto &func : noCallerVec) {
					if (func)
						func(e);
				}
			};

			// Register based on hook type
			if (key.type == HookType::Pre) {
				m_gameWrapper->HookEventWithCaller<ActorWrapper>(key.funcName, std::move(combinedCallback));
			} else {
				m_gameWrapper->HookEventWithCallerPost<ActorWrapper>(key.funcName, std::move(combinedCallback));
			}

			// Remove processed entries
			m_callbacks.erase(noCallerIt);
			m_callbacksWithCaller.erase(withCallerIt);
		}
	}

	// Process remaining pure hooks
	for (auto &pair : m_callbacks) {
		commitHook(pair);
	}
	for (auto &pair : m_callbacksWithCaller) {
		commitHook(pair);
	}

	// Clear all registered data
	clearRegisteredEvents();
	LOG("All hooks committed successfully");
}

void HookManager::commitHook(const std::pair<HookKey, std::vector<CallbackNoCaller>> &pair) {
	if (pair.first.type == HookType::Pre) {
		if (pair.second.size() == 1) {
			m_gameWrapper->HookEvent(pair.first.funcName, pair.second[0]);
		} else {
			m_gameWrapper->HookEvent(pair.first.funcName, [pair](std::string e) {
				for (const auto &func : pair.second)
					func(e);
			});
		}
	} else {
		if (pair.second.size() == 1) {
			m_gameWrapper->HookEventPost(pair.first.funcName, pair.second[0]);
		} else {
			m_gameWrapper->HookEventPost(pair.first.funcName, [pair](std::string e) {
				for (const auto &func : pair.second)
					func(e);
			});
		}
	}
}

void HookManager::commitHook(const std::pair<HookKey, std::vector<CallbackWithCaller>> &pair) {
	if (pair.first.type == HookType::Pre) {
		if (pair.second.size() == 1) {
			m_gameWrapper->HookEventWithCaller<ActorWrapper>(pair.first.funcName, pair.second[0]);
		} else {
			m_gameWrapper->HookEventWithCaller<ActorWrapper>(pair.first.funcName, [pair](ActorWrapper c, void *p, std::string e) {
				for (const auto &func : pair.second)
					func(c, p, e);
			});
		}
	} else {
		if (pair.second.size() == 1) {
			m_gameWrapper->HookEventWithCallerPost<ActorWrapper>(pair.first.funcName, pair.second[0]);
		} else {
			m_gameWrapper->HookEventWithCallerPost<ActorWrapper>(pair.first.funcName, [pair](ActorWrapper c, void *p, std::string e) {
				for (const auto &func : pair.second)
					func(c, p, e);
			});
		}
	}
}

void HookManager::clearRegisteredEvents() {
	m_registeredEvents.clear();
	m_callbacks.clear();
	m_callbacksWithCaller.clear();
}

HookManager g_hookManager{};
