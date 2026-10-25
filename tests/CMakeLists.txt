find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

# The tests instantiate the library's templates and generate code, which the header checks never do. Beyond the shared
# flags, informational /Wall warnings are disabled: class layout and implicitly deleted special members fire on every
# instantiation (padding after a trailing bool, non-assignable lambdas and mutexes) and describe no defect; optimizer
# reports (inlining, Spectre mitigation) are emitted by the backend even for external headers.
set(TEST_WARNING_OPTIONS ${SKSEHELPERS_WARNING_OPTIONS})
if (MSVC)
	list(APPEND TEST_WARNING_OPTIONS
		/wd4514 # unreferenced inline function has been removed
		/wd4623 # default constructor was implicitly defined as deleted
		/wd4625 # copy constructor was implicitly defined as deleted
		/wd4626 # assignment operator was implicitly defined as deleted
		/wd4820 # padding added after data member
		/wd5026 # move constructor was implicitly defined as deleted
		/wd5027 # move assignment operator was implicitly defined as deleted
		/wd4710 # function not inlined
		/wd4711 # function selected for automatic inline expansion
		/wd5045 # Spectre mitigation would be inserted
		/wd4868 # evaluation order in braced initializer: raised by every Catch2 TEST_CASE registration
	)
endif ()

# Runtime tests cover only headers that do not depend on CommonLib, so the executable neither includes nor links it
# and runs without the game.
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/runtime/*.cpp")

add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})
target_include_directories(${PROJECT_NAME}Tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(${PROJECT_NAME}Tests PRIVATE cxx_std_23)
target_link_libraries(${PROJECT_NAME}Tests PRIVATE Catch2::Catch2WithMain)
target_compile_options(${PROJECT_NAME}Tests PRIVATE ${TEST_WARNING_OPTIONS})
source_group("Tests" FILES ${TEST_SOURCES})

catch_discover_tests(${PROJECT_NAME}Tests)

# Compile-time tests (static_assert only) for headers that need CommonLib. Nothing is linked or run: building the target
# is the test.
file(GLOB COMPILE_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/compile/*.cpp")

add_library(${PROJECT_NAME}CompileTests OBJECT ${COMPILE_TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}CompileTests PRIVATE ${PROJECT_NAME})
target_compile_options(${PROJECT_NAME}CompileTests PRIVATE ${TEST_WARNING_OPTIONS})
source_group("Tests" FILES ${COMPILE_TEST_SOURCES})
