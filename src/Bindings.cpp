#include "pch.h"
#include "Bindings.hpp"

// ##############################################################################################################
// ########################################    BindingDetectionManager    #######################################
// ##############################################################################################################

bool BindingDetectionManager::registerBinding(const std::shared_ptr<Binding>& b)
{
	if (!b)
		return false;

	switch (b->bindingType)
	{
	case EBindingType::Combination:
		return m_combinationManager.registerBinding(b);
	case EBindingType::Sequence:
		return m_sequenceManager.registerBinding(b);
	default:
		return false;
	}
}

bool BindingDetectionManager::removeBinding(const std::shared_ptr<Binding>& b)
{
	if (!b)
		return false;

	switch (b->bindingType)
	{
	case EBindingType::Combination:
		return m_combinationManager.removeBinding(b);
	case EBindingType::Sequence:
		return m_sequenceManager.removeBinding(b);
	default:
		return false;
	}
}

// Checks button combination bindings first, then sequence bindings
std::shared_ptr<Binding> BindingDetectionManager::processKeyPress(const ButtonPress& keyPress)
{
	std::shared_ptr<Binding> binding = m_combinationManager.processKeyPress(keyPress);
	if (!binding)
		binding = m_sequenceManager.processKeyPress(keyPress);

	if (binding)
	{
		resetState(); // always reset state if a binding was triggered
		m_lastBindingActivation = std::chrono::steady_clock::now();
	}

	return binding;
}

void BindingDetectionManager::resetState()
{
	m_combinationManager.resetState();
	m_sequenceManager.resetState();
}

// ##############################################################################################################
// ########################################    SequenceBindingManager    ########################################
// ##############################################################################################################

bool SequenceBindingManager::registerBinding(const std::shared_ptr<Binding>& b)
{
	if (!b || b->bindingType != EBindingType::Sequence)
		return false;

	SequenceTrieNode* node = m_rootNode.get();
	for (const auto& buttonStr : b->buttons)
	{
		auto it = node->children.find(buttonStr);
		if (it != node->children.end())
		{
			if (it->second && it->second->binding.lock())
				return false; // sequence alr exists (either exact sequence or a prefix of this one)
			node = it->second.get();
		}
		else
		{
			node->children[buttonStr] = std::make_unique<SequenceTrieNode>();
			node                      = node->children[buttonStr].get();
		}
	}

	if (!node->children.empty())
		return false; // longer sequence alr exists (using same prefix)

	node->binding = b; // store binding (as std::weak_ptr)
	return true;
}

bool SequenceBindingManager::removeBinding(const std::shared_ptr<Binding>& b)
{
	if (!b || b->bindingType != EBindingType::Sequence)
		return false;

	std::vector<std::pair<SequenceTrieNode*, std::string>> path;
	SequenceTrieNode*                                      node = m_rootNode.get();

	for (const auto& key : b->buttons)
	{
		auto it = node->children.find(key);
		if (it == node->children.end())
			return false; // sequence doesn't exist

		path.emplace_back(node, key);
		node = it->second.get();
	}

	if (node->binding.expired())
		return false; // sequence doesn't exist as a binding

	node->binding.reset(); // clear the weak_ptr in the node

	// Prune empty nodes bottom-up
	for (auto it = path.rbegin(); it != path.rend(); ++it)
	{
		SequenceTrieNode*  parent  = it->first;
		const std::string& key     = it->second;
		auto               childIt = parent->children.find(key);

		if (childIt == parent->children.end())
			continue;

		SequenceTrieNode* child = childIt->second.get();
		if (!child->binding.expired() || !child->children.empty())
			break; // stop pruning, still in use

		parent->children.erase(childIt); // otherwise remove it
	}

	return true;
}

// Process a single button press. Returns a pointer to the binding if its button sequence has been completed within the time window
std::shared_ptr<Binding> SequenceBindingManager::processKeyPress(const ButtonPress& keyPress)
{
	auto it = m_currentNode->children.find(keyPress.buttonName);
	if (it == m_currentNode->children.end())
	{
		resetState(); // no match found -> reset
		return nullptr;
	}

	// update start time if we’re at root
	if (m_currentNode == m_rootNode.get())
		m_sequenceStart = keyPress.pressedTime;

	// check if we're within time window before advancing the node
	if (keyPress.pressedTime - m_sequenceStart > m_maxTimeWindow)
	{
		resetState();
		return nullptr;
	}

	// advance the node
	m_currentNode = it->second.get();

	// check if binding exists at node
	if (m_currentNode->binding.expired())
		return nullptr;
	else
	{
		// binding exists -> reset state and return binding ptr
		const std::shared_ptr<Binding> triggered = m_currentNode->binding.lock();
		resetState();
		return triggered;
	}
}

void SequenceBindingManager::resetState() { m_currentNode = m_rootNode.get(); }

// ##############################################################################################################
// #######################################    CombinationBindingManager    ######################################
// ##############################################################################################################

bool CombinationBindingManager::registerBinding(const std::shared_ptr<Binding>& b)
{
	if (!b || b->bindingType != EBindingType::Combination)
		return false;

	for (const auto& binding : m_bindings)
	{
		if (auto sharedBinding = binding.lock())
		{
			if (sharedBinding == b)
				return false; // binding alr registered
		}
	}

	m_bindings.push_back(b);
	return true;
}

bool CombinationBindingManager::removeBinding(const std::shared_ptr<Binding>& b)
{
	if (!b || b->bindingType != EBindingType::Combination)
		return false;

	// remove_if moves the stuff to be removed to end of vector, and returns iterator to the start of stuff to be removed
	auto it = std::remove_if(m_bindings.begin(),
	    m_bindings.end(),
	    [&b](const std::weak_ptr<Binding>& bindingView)
	    {
		    auto binding = bindingView.lock();
		    return !binding || binding == b;
	    });

	if (it == m_bindings.end())
		return false; // binding not found

	m_bindings.erase(it, m_bindings.end());
	return true;
}

std::shared_ptr<Binding> CombinationBindingManager::processKeyPress(const ButtonPress& keyPress)
{
	m_keyStates[keyPress.buttonName] = true;

	for (const auto& bindingView : m_bindings)
	{
		if (!isBindingTriggered(bindingView))
			continue;

		return bindingView.lock();
	}

	return nullptr;
}

bool CombinationBindingManager::isBindingTriggered(const std::weak_ptr<Binding>& bindingView)
{
	auto binding = bindingView.lock();
	if (!binding)
		return false;

	for (const std::string& keyName : binding->buttons)
	{
		auto it = m_keyStates.find(keyName);
		if (it == m_keyStates.end() || !it->second)
			return false;
	}

	return true;
}

void CombinationBindingManager::resetState()
{
	for (auto& pair : m_keyStates)
		pair.second = false;
}
