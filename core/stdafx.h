#pragma once

#ifdef _WIN32
#pragma warning( disable : 35038)
#include <WinSDKVer.h>
#define WINVER 0x0601
#define _WIN32_WINDOWS 0x0601
#define _WIN32_WINNT 0x0601
#define NTDDI_VERSION 0x06010000

#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_LEAN_AND_MEAN

#include "win32/targetver.h"
#include <boost/asio.hpp>
#include <Windows.h>
#include <VersionHelpers.h>
#include <Shlobj.h>
#include <atlbase.h>
#include <atlstr.h>
#include <Psapi.h>
#pragma warning( error : 35038)
#endif //WIN32

#include <cassert>
#include <assert.h>
#if !defined(_WIN32) && !defined(ABORT_ON_ASSERT) && defined(DEBUG)
#define im_assert(condition) \
do { if(!(condition)){ std::cerr << "ASSERT FAILED: " << #condition << " " << __FILE__ << ":" << __LINE__ << std::endl; } } while (0)
#else
#define im_assert(condition) do { assert(condition); } while (0)
#endif

#include <algorithm>
#include <numeric>
#include <atomic>
#include <ctime>
#include <codecvt>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iterator>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <optional>
#include <type_traits>
#include <charconv>
#include <typeinfo>

#if defined(_WIN32) || defined(__linux__)
#include <filesystem>
#endif

#ifndef _WIN32
#include <boost/asio.hpp>
#endif //_WIN32
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include <boost/locale/generator.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/config.hpp>
#include <boost/thread/thread.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/thread.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "../common.shared/common.h"
#include "../common.shared/typedefs.h"

#include "tools/tlv.h"
#include "tools/binary_stream.h"
#include "tools/binary_stream_reader.h"
#include "tools/scope.h"
#include "tools/strings.h"

#undef min
#undef max
#undef small

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "../common.shared/constants.h"

using rapidjson_allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

#ifdef __APPLE__
#   if defined(DEBUG) || defined(_DEBUG)
//#       define DEBUG__OUTPUT_NET_PACKETS
#   endif // defined(DEBUG) || defined(_DEBUG)
#endif // __APPLE__

#include "priority.h"
