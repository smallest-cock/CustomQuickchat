#pragma once
#include "Structs.hpp"
#include <string>
#include <unordered_map>

struct SequenceTrieNode
{
	std::unordered_map<std::string, std::unique_ptr<SequenceTrieNode>> children;
	std::weak_ptr<Binding>                                             binding;
};

class SequenceBindingManager
{
	std::unique_ptr<SequenceTrieNode> m_rootNode;
	SequenceTrieNode*                 m_currentNode;

	// time window state
	std::chrono::steady_clock::time_point m_sequenceStartTime{};
	std::chrono::duration<double>         m_maxTimeWindow{2.0};

public:
	SequenceBindingManager() : m_rootNode(std::make_unique<SequenceTrieNode>()), m_currentNode(m_rootNode.get()) {}

	bool                     registerBinding(const std::shared_ptr<Binding>& b);
	bool                     removeBinding(const std::shared_ptr<Binding>& b);
	std::shared_ptr<Binding> processKeyPress(const ButtonPress& keyPress);
	void                     resetState();
	void                     clearBindings();

	void setMaxTimeWindow(const std::chrono::duration<double>& duration) { m_maxTimeWindow = duration; }
};

class CombinationBindingManager
{
	std::unordered_map<std::string, bool> m_keyStates;
	std::vector<std::weak_ptr<Binding>>   m_bindings;

private:
	bool isBindingTriggered(const std::weak_ptr<Binding>& bindingView);

public:
	bool                     registerBinding(const std::shared_ptr<Binding>& b);
	bool                     removeBinding(const std::shared_ptr<Binding>& b);
	std::shared_ptr<Binding> processKeyPress(const ButtonPress& keyPress);
	void                     resetState();
	void                     updateKeyState(const std::string& keyName, bool pressed) { m_keyStates[keyName] = pressed; }
	void                     clearBindings() { m_bindings.clear(); }
};

class BindingDetectionManager
{
	CombinationBindingManager             m_combinationManager;
	SequenceBindingManager                m_sequenceManager;
	std::chrono::steady_clock::time_point m_lastBindingActivation;

public:
	bool                     registerBinding(const std::shared_ptr<Binding>& b);
	bool                     removeBinding(const std::shared_ptr<Binding>& b);
	std::shared_ptr<Binding> processKeyPress(const ButtonPress& keyPress);
	void                     resetState();
	void                     clearBindings();

	std::chrono::steady_clock::time_point getLastBindingActivation() const { return m_lastBindingActivation; }
	void setLastBindingActivation(const std::chrono::steady_clock::time_point& time) { m_lastBindingActivation = time; }
	void updateKeyState(const std::string& keyName, bool pressed) { m_combinationManager.updateKeyState(keyName, pressed); }
	void setSequenceMaxTimeWindow(double duration) { m_sequenceManager.setMaxTimeWindow(std::chrono::duration<double>(duration)); }
};