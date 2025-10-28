#pragma once
#define VERSION_MAJOR 1
#define VERSION_MINOR 10
#define VERSION_PATCH 4
#define VERSION_BUILD 0

#define VERSION_SUFFIX "" // can be "" if no suffix

#define stringify(a) stringify_(a)
#define stringify_(a) #a

#define VERSION_STR stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) VERSION_SUFFIX
