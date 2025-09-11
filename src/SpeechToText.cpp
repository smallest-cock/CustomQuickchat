#include "pch.h"
#ifndef USE_SPEECH_TO_TEXT
#include "CustomQuickchat.hpp"
#include "Macros.hpp"

void CustomQuickchat::no_speech_to_text_warning()
{
	std::string message = "This version doesnt support speech-to-text. You can find that version on the github Releases page";
	NotifyAndLog("Speech-To-Text", message, 5);
}
#endif
