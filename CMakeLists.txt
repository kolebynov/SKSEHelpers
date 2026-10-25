cmake_minimum_required(VERSION 3.21)

# SKSEHelpers: shared helpers for SKSE plugins built on CommonLibSSE-NG.
#
# CommonLibSSE-NG is not vendored. It is resolved in this order:
#   1. An existing CommonLibSSE::CommonLibSSE target (the consumer already added CommonLib).
#   2. COMMONLIB_PATH (cache variable or environment variable) pointing at a CommonLibSSE-NG checkout,
#      which is added via add_subdirectory().
#
# Consumer usage:
#   add_subdirectory("C:/work/SKSEHelpers" "${CMAKE_BINARY_DIR}/SKSEHelpers")
#   target_link_libraries(MyPlugin PRIVATE SKSEHelpers::SKSEHelpers)
# The consumer's vcpkg manifest must provide CommonLibSSE-NG's dependencies plus `minhook`.

project(
	SKSEHelpers
	VERSION 1.0.0
	LANGUAGES CXX
)

if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not allowed.")
endif ()

# ---- CommonLibSSE-NG ----

if (NOT TARGET CommonLibSSE::CommonLibSSE)
	if (NOT DEFINED COMMONLIB_PATH AND DEFINED ENV{COMMONLIB_PATH})
		set(COMMONLIB_PATH "$ENV{COMMONLIB_PATH}")
	endif ()
	set(COMMONLIB_PATH "${COMMONLIB_PATH}" CACHE PATH "Path to a CommonLibSSE-NG checkout.")

	if (NOT COMMONLIB_PATH)
		message(FATAL_ERROR "COMMONLIB_PATH is not set. Configure with -DCOMMONLIB_PATH=<path to CommonLibSSE-NG>.")
	endif ()

	cmake_path(ABSOLUTE_PATH COMMONLIB_PATH NORMALIZE OUTPUT_VARIABLE _skse_helpers_commonlib_dir)
	if (NOT EXISTS "${_skse_helpers_commonlib_dir}/CMakeLists.txt")
		message(FATAL_ERROR "COMMONLIB_PATH '${_skse_helpers_commonlib_dir}' does not contain a CMakeLists.txt.")
	endif ()

	message(STATUS "SKSEHelpers: using CommonLibSSE-NG from ${_skse_helpers_commonlib_dir}")
	set(BUILD_TESTS OFF)
	add_subdirectory("${_skse_helpers_commonlib_dir}" "${CMAKE_CURRENT_BINARY_DIR}/CommonLibSSE" EXCLUDE_FROM_ALL)
endif ()

find_package(minhook CONFIG REQUIRED)

# ---- Library ----

# Header-only: an INTERFACE library carrying include path, dependencies and definitions.
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

target_link_libraries(
	${PROJECT_NAME}
	INTERFACE
		CommonLibSSE::CommonLibSSE
		minhook::minhook
)

# Hooks/Hooks.h includes MinHook.h, which pulls in <Windows.h>; keep its min/max and extra headers out of consumers.
target_compile_definitions(
	${PROJECT_NAME}
	INTERFACE
		NOMINMAX
		WIN32_LEAN_AND_MEAN
)

# Every warning is enabled and fatal for this library's code. Angle-bracket includes (STL, CommonLib, MinHook,
# Windows SDK, Catch2) are treated as external and silenced, so only warnings from SKSEHelpers' own files are reported.
# Applied to the header checks and the tests.
set(SKSEHELPERS_WARNING_OPTIONS)
if (MSVC)
	set(SKSEHELPERS_WARNING_OPTIONS /utf-8 /permissive- /Wall /WX /external:anglebrackets /external:W0)
endif ()

# Headers are only compiled by consumers, so compile each one on its own here as well; this keeps every header
# self-contained and subject to the warning flags above. Enabled by default only for standalone builds.
option(SKSEHELPERS_HEADER_CHECKS "Compile every header in its own translation unit." ${PROJECT_IS_TOP_LEVEL})
if (SKSEHELPERS_HEADER_CHECKS)
	set(HEADER_CHECK_SOURCES)
	foreach (header IN LISTS HEADERS)
		file(RELATIVE_PATH header_rel "${CMAKE_CURRENT_SOURCE_DIR}/src" "${header}")
		string(MAKE_C_IDENTIFIER "${header_rel}" header_id)
		set(header_check "${CMAKE_CURRENT_BINARY_DIR}/HeaderChecks/${header_id}.cpp")
		file(CONFIGURE OUTPUT "${header_check}" CONTENT "#include \"${header_rel}\"\n")
		list(APPEND HEADER_CHECK_SOURCES "${header_check}")
	endforeach ()
	source_group("Header Checks" FILES ${HEADER_CHECK_SOURCES})
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADERS})

	add_library(${PROJECT_NAME}HeaderChecks OBJECT ${HEADER_CHECK_SOURCES} ${HEADERS} .clang-format .editorconfig)
	target_link_libraries(${PROJECT_NAME}HeaderChecks PRIVATE ${PROJECT_NAME})
	target_compile_options(${PROJECT_NAME}HeaderChecks PRIVATE ${SKSEHELPERS_WARNING_OPTIONS})
endif ()

# Unit tests need Catch2 from the vcpkg manifest feature "tests" (enabled by the "default" configure preset).
option(SKSEHELPERS_TESTS "Build the unit tests." ${PROJECT_IS_TOP_LEVEL})
if (SKSEHELPERS_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif ()
