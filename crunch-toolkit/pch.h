// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here

// general external stuff
#include <cmath>
#include <limits>
#include <vector>
#include <list>
#include <memory>
#include <utility>
#include <future>
#include <algorithm>
#include <thread>
#include <string>
#include <filesystem>
#include <queue>
#include <optional>
#include <iostream>

//slippc
#include "analysis.h"
#include "analyzer.h"
#include "compressor.h"
#include "enums.h"
#include "gecko-legacy.h"
#include "lzma.h"
#include "parser.h"
#include "portable-file-dialogs.h"
#include "replay.h"
#include "schema.h"
#include "shiftjis.h"
#include "util.h"

#endif //PCH_H
