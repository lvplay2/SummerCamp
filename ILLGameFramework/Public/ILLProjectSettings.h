// Copyright (C) 2015-2018 IllFonic, LLC.

// This file contains defines to modify the IGF to better suit a given project
// Override the definitions below in ILLProjectOverrides.h and do not integrate those changes out
#include "ILLProjectOverrides.h"

// Include build defines so it can also override these settings
#include "ILLBuildDefines.h"

// Disallow GameLift dedicated servers by default
#ifndef ILLBUILD_WITH_GAMELIFT
# define ILLBUILD_WITH_GAMELIFT 1
#endif

// Allow GameLift dedicated server management and client-side matchmaking?
#ifndef ALLOW_GAMELIFT_PLUGIN
# define ALLOW_GAMELIFT_PLUGIN (ILLBUILD_WITH_GAMELIFT && HAS_GAMELIFTSERVERSDK_PLUGIN)
#endif

// Allow GameLiftServerSDK usage?
#ifndef USE_GAMELIFT_SERVER_SDK
# define USE_GAMELIFT_SERVER_SDK (ALLOW_GAMELIFT_PLUGIN && WITH_GAMELIFT)
#endif

// Attempt to use HTTP matchmaking when GameLift is present
#ifndef MATCHMAKING_SUPPORTS_HTTP_API
# define MATCHMAKING_SUPPORTS_HTTP_API (ALLOW_GAMELIFT_PLUGIN)
#endif

// Should we require network encryption on desktop?
#ifndef ILLNETENCRYPTION_DESKTOP_REQUIRED
# define ILLNETENCRYPTION_DESKTOP_REQUIRED 1
#endif
