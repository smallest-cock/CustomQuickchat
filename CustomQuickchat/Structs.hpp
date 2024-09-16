#pragma once
#include "pch.h"



// ------------------------- for imgui -------------------------

const std::vector<std::string> possibleBindingTypes =
{
	"button combination",
	"button sequence"
};

const std::vector<std::string> possibleChatModes =
{
	"lobby",
	"team",
	"party"
};

// -------------------------------------------------------------


enum class BindingType : uint8_t
{
	Combination =	0,
	Sequence =		1
};


struct ButtonPress
{
	std::string buttonName;
	std::chrono::steady_clock::time_point pressedTime;


	// default (empty)
	ButtonPress()
		: buttonName(std::string()), pressedTime(std::chrono::steady_clock::time_point()) {}

	// specific button press event
	ButtonPress(const std::string& button, const std::chrono::steady_clock::time_point& time)
		: buttonName(button), pressedTime(time) {}
	
	void Reset(const std::chrono::steady_clock::time_point& epochTime)
	{
		buttonName.clear();
		pressedTime = epochTime;
	}
};


struct Binding
{
	std::string chat;
	EChatChannel chatMode = EChatChannel::EChatChannel_Match;
	BindingType bindingType = BindingType::Combination;
	std::vector<std::string> buttons;
	ButtonPress firstButtonState;


	bool ShouldBeTriggered(
		const ButtonPress& buttonEvent,
		const std::unordered_map<std::string, bool>& keyStates,
		const std::chrono::steady_clock::time_point& lastChatSent,
		const std::chrono::steady_clock::time_point& epochTime,
		const std::chrono::duration<double>& minDelayBetweenBindings,
		const std::chrono::duration<double>& maxTimeWindow)
	{
		switch (bindingType)
		{
		case BindingType::Combination:
			return CheckCombination(buttonEvent, keyStates, lastChatSent, minDelayBetweenBindings);
		case BindingType::Sequence:
			return CheckSequence(buttonEvent, lastChatSent, epochTime, minDelayBetweenBindings, maxTimeWindow);
		default:
			return false;	// if there's no valid binding type for some reason
		}
	}
	
	bool CheckCombination(
		const ButtonPress& buttonEvent,
		const std::unordered_map<std::string, bool>& keyStates,
		const std::chrono::steady_clock::time_point& lastBindingActivated,
		const std::chrono::duration<double>& minDelayBetweenBindings)
	{
		if (buttons.empty()) return false;

		for (const std::string& button : buttons)
		{
			if (keyStates.contains(button))
			{
				if (!keyStates.at(button)) return false;
			}
		}

		// check if event happened AFTER minBindingDelay
		return buttonEvent.pressedTime > lastBindingActivated + minDelayBetweenBindings;
	}

	bool CheckSequence(
		const ButtonPress& buttonEvent,
		const std::chrono::steady_clock::time_point& lastChatSent,
		const std::chrono::steady_clock::time_point& epochTime,
		const std::chrono::duration<double>& minDelayBetweenBindings,
		const std::chrono::duration<double>& maxTimeWindow)
	{
		if (buttons.size() < 2) return false;	// exit if there's not at least 2 buttons in binding

		bool button1Pressed = buttonEvent.buttonName == buttons[0];
		bool button2Pressed = buttonEvent.buttonName == buttons[1];

		if (!button1Pressed && !button2Pressed) return false;	// early exit if no buttons from binding have been pressed

		// if first button press data is empty...
		if (firstButtonState.buttonName.empty() || firstButtonState.pressedTime == epochTime)
		{
			if (button1Pressed)
				firstButtonState = buttonEvent;		// update first button press data then exit
			return false;
		}

		// if first button press data exists.......
		
		// if first button press data is too old... reset or update it, then exit
		if (buttonEvent.pressedTime > firstButtonState.pressedTime + maxTimeWindow)
		{
			if (button1Pressed)
				firstButtonState = buttonEvent;		// update first button press data
			else
				firstButtonState.Reset(epochTime);	// reset info bc 1st button doesn't match
			return false;
		}

		// if first button press data is still valid.......

		if (!button2Pressed) return false;

		// make sure 2nd button pressed in appropriate time window (AFTER minBindingDelay and BEFORE sequenceTimeWindow)
		bool correct1stButtonPressed = firstButtonState.buttonName == buttons[0];
		bool button2PressedLateEnough = buttonEvent.pressedTime > firstButtonState.pressedTime + minDelayBetweenBindings;

		if (correct1stButtonPressed)
		{
			if (button2PressedLateEnough)
			{
				firstButtonState.Reset(epochTime);
				return true;
			} 

			firstButtonState.Reset(epochTime);	// binding was triggered too early, just reset it (bc it prolly wasn't meant to be triggered)
		}

		return false;
	}
};


struct VariationList
{
	std::string listName;
	std::string unparsedString;
	std::vector<std::string> wordList;
	std::vector<std::string> shuffledWordList;
	int nextUsableIndex = 0;
};


struct Rank
{
	int matches;
	std::string div;
	std::string tier;
	int mmr;
};


struct ChatterRanks
{
	std::string playerName;
	std::unordered_map <std::string, Rank> ranks;
};
