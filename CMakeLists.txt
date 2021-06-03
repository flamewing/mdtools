cmake_minimum_required(VERSION 3.12)

project(mdtools
    VERSION 0.2.0
    DESCRIPTION "Assorted compression formats for the Sega Mega Drive"
    LANGUAGES CXX ASM
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED ENV{MSYSTEM_PREFIX})
        set(CMAKE_INSTALL_PREFIX "$ENV{MSYSTEM_PREFIX}" CACHE PATH "Default install prefix" FORCE)
    endif(DEFINED ENV{MSYSTEM_PREFIX})
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
include(GNUInstallDirs)

find_package(Boost 1.54 REQUIRED)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/mdcomp/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

add_subdirectory(mdcomp)

include(CheckCXXCompilerFlag)

function(add_compile_options_safe FLAG)
    string(REGEX REPLACE "[-=+]" "" FLAG_NO_SIGNS ${FLAG}) # <- The variable recieving the result of the test can't have those signs in its name
    check_cxx_compiler_flag(${FLAG} CXX_COMPILER_SUPPORTS_${FLAG_NO_SIGNS})
    if(CXX_COMPILER_SUPPORTS_${FLAG_NO_SIGNS})
        add_compile_options("${FLAG}")
    endif(CXX_COMPILER_SUPPORTS_${FLAG_NO_SIGNS})
endfunction()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (${FORCE_COLORED_OUTPUT})
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-fdiagnostics-color=always)
        else()
            add_compile_options(-fcolor-diagnostics)
        endif()
    endif()
    if (WARNINGS_ARE_ERRORS)
        add_compile_options(-Werror)
        add_compile_options_safe(-pedantic-errors)
    endif()
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        add_compile_options_safe(-Og)
        add_compile_options_safe(-g3)
    endif()
    add_compile_options("-Wall")
    add_compile_options("-Wextra")
    add_compile_options("-pedantic")
    add_compile_options("-Wc++14-compat")
    add_compile_options("-Wc++17-compat")
    add_compile_options_safe("-Wc++20-compat")
    add_compile_options_safe("-Wc++14-compat-pedantic")
    add_compile_options_safe("-Wc++17-compat-pedantic")
    add_compile_options_safe("-Wc++20-compat-pedantic")
    add_compile_options_safe("-Walloc-zero")
    add_compile_options("-Walloca")
    add_compile_options_safe("-Wanalyzer-too-complex")
    add_compile_options_safe("-Warith-conversion")
    add_compile_options_safe("-Warray-bounds-pointer-arithmetic")
    add_compile_options_safe("-Wassign-enum")
    add_compile_options_safe("-Wbad-function-cast")
    add_compile_options_safe("-Wbitwise-op-parentheses")
    add_compile_options_safe("-Wbraced-scalar-init")
    add_compile_options_safe("-Wbridge-cast")
    add_compile_options("-Wcast-align")
    add_compile_options_safe("-Wcast-align=strict")
    add_compile_options("-Wcast-qual")
    add_compile_options_safe("-Wchar-subscripts")
    add_compile_options_safe("-Wcomma")
    add_compile_options_safe("-Wcomma-subscript")
    add_compile_options_safe("-Wcomment")
    add_compile_options_safe("-Wconditionally-supported")
    add_compile_options_safe("-Wcovered-switch-default")
    add_compile_options("-Wctor-dtor-privacy")
    add_compile_options("-Wdate-time")
    add_compile_options_safe("-Wdeprecated-copy")
    add_compile_options_safe("-Wdeprecated-copy-dtor")
    add_compile_options_safe("-Wdeprecated-dynamic-exception-spec")
    add_compile_options("-Wdisabled-optimization")
    add_compile_options("-Wdouble-promotion")
    add_compile_options_safe("-Wduplicated-branches")
    add_compile_options_safe("-Wduplicated-cond")
    add_compile_options_safe("-Wempty-init-stmt")
    add_compile_options("-Wextra-semi")
    add_compile_options_safe("-Wfor-loop-analysis")
    add_compile_options("-Wformat-nonliteral")
    add_compile_options("-Wformat-security")
    add_compile_options_safe("-Wformat-signedness")
    add_compile_options("-Wformat-y2k")
    add_compile_options_safe("-Wfour-char-constants")
    add_compile_options_safe("-Wgcc-compat")
    add_compile_options_safe("-Wheader-hygiene")
    add_compile_options_safe("-Widiomatic-parentheses")
    add_compile_options_safe("-Wint-in-bool-context")
    add_compile_options("-Winvalid-pch")
    add_compile_options_safe("-Wlogical-op")
    add_compile_options_safe("-Wlogical-op-parentheses")
    add_compile_options_safe("-Wmany-braces-around-scalar-init")
    add_compile_options_safe("-Wmisleading-indentation")
    add_compile_options_safe("-Wmismatched-tags")
    # add_compile_options("-Wmissing-braces")
    add_compile_options("-Wmissing-include-dirs")
    add_compile_options_safe("-Wmove")
    add_compile_options("-Wmultichar")
    add_compile_options_safe("-Wnoexcept")
    add_compile_options("-Wnon-virtual-dtor")
    add_compile_options("-Wnull-dereference")
    add_compile_options("-Wold-style-cast")
    add_compile_options_safe("-Wover-aligned")
    add_compile_options("-Woverloaded-virtual")
    add_compile_options("-Wpacked")
    add_compile_options("-Wredundant-decls")
    add_compile_options_safe("-Wredundant-tags")
    add_compile_options("-Wregister")
    add_compile_options_safe("-Wshadow=compatible-local")
    add_compile_options_safe("-Wshadow=local")
    add_compile_options_safe("-Wshadow-field")
    add_compile_options_safe("-Wshadow-field-in-constructor")
    add_compile_options_safe("-Wshadow-field-in-constructor-modified")
    add_compile_options_safe("-Wshadow-ivar")
    add_compile_options_safe("-Wshadow-uncaptured-local")
    # add_compile_options("-Wsign-promo")
    add_compile_options("-Wstack-protector")
    add_compile_options_safe("-Wstrict-null-sentinel")
    add_compile_options_safe("-Wstring-conversion")
    add_compile_options_safe("-Wstring-plus-char")
    add_compile_options_safe("-Wsuggest-attribute=cold")
    add_compile_options_safe("-Wsuggest-attribute=const")
    add_compile_options_safe("-Wsuggest-attribute=format")
    add_compile_options_safe("-Wsuggest-attribute=malloc")
    add_compile_options_safe("-Wsuggest-attribute=noreturn")
    add_compile_options_safe("-Wsuggest-attribute=pure")
    add_compile_options_safe("-Wsuggest-final-methods")
    add_compile_options_safe("-Wsuggest-final-types")
    add_compile_options_safe("-Wsuggest-destructor-override")
    add_compile_options_safe("-Wsuggest-override")
    # add_compile_options("-Wswitch-default")
    add_compile_options("-Wswitch-enum")
    add_compile_options("-Wsynth")
    add_compile_options_safe("-Wtrampolines")
    add_compile_options("-Wundef")
    add_compile_options_safe("-Wunused-label")
    add_compile_options_safe("-Wunused-lambda-capture")
    add_compile_options_safe("-Wunused-local-typedef")
    add_compile_options("-Wunused-macros")
    add_compile_options_safe("-Wuseless-cast")
    add_compile_options_safe("-Wvector-conversion")
    add_compile_options_safe("-Wvector-operation-performance")
    add_compile_options_safe("-Wvirtual-inheritance")
    add_compile_options_safe("-Wvolatile")
    add_compile_options("-Wzero-as-null-pointer-constant")
    add_compile_options_safe("-Warray-bounds=2")
    add_compile_options_safe("-Wattribute-alias=2")
    add_compile_options_safe("-Wcatch-value=3")
    add_compile_options_safe("-Wformat-overflow=2")
    add_compile_options_safe("-Wformat-truncation=2")
    add_compile_options_safe("-Wformat=2")
    add_compile_options("-Wimplicit-fallthrough")
    add_compile_options_safe("-Wimplicit-fallthrough=3")
    add_compile_options_safe("-Wplacement-new=2")
    add_compile_options_safe("-Wshift-overflow=2")
    add_compile_options_safe("-Wstrict-aliasing=3")
    add_compile_options("-Wstrict-overflow=2")
    add_compile_options_safe("-Wstringop-overflow=4")
    add_compile_options("-Wunused-const-variable")
    add_compile_options_safe("-Wunused-const-variable=1")
elseif (MSVC)
    if (WARNINGS_ARE_ERRORS)
        add_compile_options("/WX")
    endif()
    add_compile_options("/W4")
    add_compile_options("/wd4018") # warning C4018: '>': signed/unsigned mismatch
    add_compile_options("/wd4127") # warning C4127: conditional expression is constant
    add_compile_options("/wd4244") # warning C4244: 'initializing': conversion from 'int' to 'char', possible loss of data
    add_compile_options("/wd4251")
    # Clang: -Wshorten-64-to-32 -Wimplicit-int-conversion
    add_compile_options("/wd4267") # warning C4267: 'return': conversion from 'size_t' to 'int', possible loss of data
    add_compile_options("/wd4389") # warning C4389: '==': signed/unsigned mismatch
    add_compile_options("/wd4482")
    add_compile_options("/wd4512")
    add_compile_options("/wd4701") # warning C4701: potentially uninitialized local variable 'err' used
    add_compile_options("/wd4706") # warning C4706: assignment within conditional expression
    add_compile_options("/wd4800") # warning C4800: 'const SymbolDatabase *' : forcing value to bool 'true' or 'false' (performance warning)
endif()

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    # If Microsoft SDK is installed create script run-msbuild.bat that
    # calls SetEnv.cmd to set up build environment and runs msbuild.
    # It is useful when building Visual Studio projects with the SDK
    # toolchain rather than Visual Studio.
    include(FindSetEnv)
    if (WINSDK_SETENV)
        set(MSBUILD_SETUP "call \"${WINSDK_SETENV}\"")
    endif ()
    # Set FrameworkPathOverride to get rid of MSB3644 warnings.
    set(netfxpath "C:\\Program Files\\Reference Assemblies\\Microsoft\\Framework\\.NETFramework\\v4.0")
    file(WRITE run-msbuild.bat "
    ${MSBUILD_SETUP}
    ${CMAKE_MAKE_PROGRAM} -p:FrameworkPathOverride=\"${netfxpath}\" %*")
endif()

set(COMMON_HEADERS
    "include/mdtools/ignore_unused_variable_warning.hh"
)

set(MAPPING_HEADERS
    "include/mdtools/singledplc.hh"
    "include/mdtools/framedplc.hh"
    "include/mdtools/dplcfile.hh"
    "include/mdtools/singlemapping.hh"
    "include/mdtools/framemapping.hh"
    "include/mdtools/mappingfile.hh"
)

set(SMPS_HEADERS
    "include/mdtools/fmvoice.hh"
    "include/mdtools/songtrack.hh"
)

set(VRAM_HEADERS
    "include/mdtools/pattern_name.hh"
    "include/mdtools/pattern_name_table.hh"
    "include/mdtools/tile.hh"
    "include/mdtools/vram.hh"
)

set(VRASSTRACK_HEADERS
    "include/mdtools/ssvram.hh"
    "include/mdtools/sstrack.hh"
)

# Dummy library for generating compile_commands.json that
# sets flags for headers without corresponding cc files.
add_library(dummy-mdtools
    "src/lib/fmvoice.cc"
    "src/lib/ignore_unused_variable_warning.cc"
    "src/lib/pattern_name.cc"
    "src/lib/pattern_name_table.cc"
    "src/lib/songtrack.cc"
    "src/lib/tile.cc"
    "src/lib/vram.cc"
    "${VRAM_HEADERS}"
    "${SMPS_HEADERS}"
    "${COMMON_HEADERS}"
)
target_include_directories(dummy-mdtools
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(dummy-mdtools
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/mdcomp/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(dummy-mdtools
    INTERFACE
        mdcomp::bigendian_io
)
set_target_properties(dummy-mdtools
    PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        POSITION_INDEPENDENT_CODE ON
)

function(define_exe TARGETNAME SRCFILE LIBNAME EXENAME)
    add_executable(${TARGETNAME}
        "${SRCFILE}"
    )
    target_include_directories(${TARGETNAME}
        PUBLIC
            "include"
            "fmt/include"
            "mdcomp/include"
    )
    if(NOT "${LIBNAME}" STREQUAL "")
        target_link_libraries(${TARGETNAME} PUBLIC ${LIBNAME})
    endif()
    set_target_properties(${TARGETNAME}
        PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
            POSITION_INDEPENDENT_CODE ON
            OUTPUT_NAME ${EXENAME}
    )
endfunction()

add_library(mappings
    SHARED
        "src/lib/singledplc.cc"
        "src/lib/framedplc.cc"
        "src/lib/dplcfile.cc"
        "src/lib/singlemapping.cc"
        "src/lib/framemapping.cc"
        "src/lib/mappingfile.cc"
        "${MAPPING_HEADERS}"
        "${COMMON_HEADERS}"
)
add_library(mdtools::mappings ALIAS mappings)
target_include_directories(mappings
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/mdcomp/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(mappings
    INTERFACE
        mdcomp::bigendian_io
)
set_target_properties(mappings
    PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        POSITION_INDEPENDENT_CODE ON
        PUBLIC_HEADER "${MAPPING_HEADERS};${COMMON_HEADERS}"
)

add_library(sstrack
    SHARED
        "src/lib/ssvram.cc"
        "src/lib/sstrack.cc"
        "${VRASSTRACK_HEADERS}"
        "${VRAM_HEADERS}"
        "${COMMON_HEADERS}"
)
add_library(mdtools::sstrack ALIAS sstrack)
target_include_directories(sstrack
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(sstrack
    INTERFACE
        mdcomp::bigendian_io
)
target_link_libraries(sstrack
    PUBLIC
        mdcomp::kosinski
        mdcomp::enigma
)
set_target_properties(sstrack
    PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        POSITION_INDEPENDENT_CODE ON
        PUBLIC_HEADER "${VRASSTRACK_HEADERS};${VRAM_HEADERS};${COMMON_HEADERS}"
)

set(VOICEDUMPER_SOURCES
    "src/tools/fmvoice.cc"
    "src/tools/voice_dumper.cc"
)
define_exe(voice_dumper   "${VOICEDUMPER_SOURCES}"      ""                                          voice_dumper)
define_exe(chunk_census   "src/tools/chunk_census.cc"   "mdcomp::kosinski"                          chunk_census)
define_exe(split_art      "src/tools/split_art.cc"      "mappings;mdcomp::comper;mdcomp::kosinski"  split_art)
define_exe(chunk_splitter "src/tools/chunk_splitter.cc" ""                                          chunk_splitter)
define_exe(ssexpand       "src/tools/ssexpand.cc"       "sstrack;mdcomp::enigma;mdcomp::kosinski"   ssexpand)
set(SMPS2ASM_SOURCES
    "src/tools/smps2asm.cc"
    "src/tools/fmvoice.cc"
    "src/tools/songtrack.cc"
)
define_exe(smps2asm       "${SMPS2ASM_SOURCES}"         "mdcomp::saxman"                            smps2asm)
define_exe(recolor_art    "src/tools/recolor_art.cc"    "mdcomp::nemesis;mdcomp::kosinski"          recolor_art)
define_exe(mapping_tool   "src/tools/mapping_tool.cc"   "mappings"                                  mapping_tool)
define_exe(plane_map      "src/tools/plane_map.cc"      "mdcomp::enigma"                            plane_map)
define_exe(enitool        "src/tools/enitool.cc"        "mdcomp::enigma"                            enitool)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    mdtoolsConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(
    TARGETS
        mappings
        sstrack
        voice_dumper
        chunk_census
        split_art
        chunk_splitter
        ssexpand
        smps2asm
        recolor_art
        mapping_tool
        plane_map
        enitool
    EXPORT
        mdtoolsConfig
    LIBRARY
        COMPONENT Libraries
        DESTINATION lib
        NAMELINK_COMPONENT Development
    RUNTIME
        COMPONENT Tools
        DESTINATION bin
    PUBLIC_HEADER
        COMPONENT Development
        DESTINATION include/mdtools
)

export(
    TARGETS
    mappings
    sstrack
NAMESPACE
        mdtools::
    FILE
        mdtoolsConfig.cmake
)

set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(PACKAGE mdtools)
