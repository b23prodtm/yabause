project(m68kmake)

cmake_minimum_required(VERSION 2.6)

add_executable(m68kmake m68kmake.c)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (MSVC)

option(YAB_FORCE_armv5tel "Compile for ARM Cross Compile")
if (YAB_FORCE_ARM)
	set(CMAKE_SYSTEM_PROCESSOR "armv5tel")
endif(YAB_FORCE_ARM)

option(YAB_FORCE_ARMV7 "Compile for ARMv7l Cross Compile")
if (YAB_FORCE_ARMV7)
	set(CMAKE_SYSTEM_PROCESSOR "armv7l")
endif(YAB_FORCE_ARMV7)

option(YAB_FORCE_ARMV8 "Compile for ARM64 Cross Compile")
if (YAB_FORCE_ARMV8)
	set(CMAKE_SYSTEM_PROCESSOR "arm64")
endif(YAB_FORCE_ARMV8)

add_custom_command(OUTPUT m68kops.h
                   COMMAND m68kmake ${CMAKE_CURRENT_BINARY_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/m68k_in.c
                   DEPENDS m68kmake.c
                   COMMENT "Generating musashi 68k core"
                   VERBATIM)

add_custom_target(musashi ALL DEPENDS m68kops.h)
