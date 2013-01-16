#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "debug.h"

#define STATUS_OK (0)
#define STATUS_FAILED (-1)

#include "workers.h"
#include "sampler.h"
#include "server.h"
#include "storage.h"
#include "settings.h"
#include "key.h"

#define TESTING_MODE 0


#endif
