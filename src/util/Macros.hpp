#ifndef MACROS_H
#define MACROS_H

#define DELAY(delay, body, ...) gameWrapper->SetTimeout([ this, ##__VA_ARGS__ ](GameWrapper * gw) body, delay)

#define GAME_THREAD_EXECUTE(body, ...)                                                                                                     \
	do                                                                                                                                     \
	{                                                                                                                                      \
		gameWrapper->Execute([ this, ##__VA_ARGS__ ](GameWrapper * gw) body);                                                              \
	} while (0)

#define RUN_COMMAND(cvar) cvarManager->executeCommand(cvar.name);

#define DELAY_RUN_COMMAND(cvar, delaySeconds)                                                                                              \
	gameWrapper->SetTimeout([this](GameWrapper *gw) { cvarManager->executeCommand(cvar.name); }, delaySeconds);

#endif // MACROS_H
