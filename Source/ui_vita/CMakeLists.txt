cmake_minimum_required(VERSION 3.5)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(Play_Vita)

add_definitions(-DPLAY_VERSION="${PROJECT_Version}")

set(BASE_FLAGS "${BASE_FLAGS} -mthumb -mthumb-interwork")
set(BASE_FLAGS "${BASE_FLAGS} -march=armv7-a -mtune=cortex-a9 -mfloat-abi=hard -ffast-math")
set(BASE_FLAGS "${BASE_FLAGS} -mfpu=neon -ftree-vectorize -fsingle-precision-constant")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${BASE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BASE_FLAGS}")

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

if(NOT TARGET gsh_gxm)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_GXM
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_GXM
	)
endif()
list(INSERT PROJECT_LIBS 0 gsh_gxm)

set(VITA_APP_NAME "Play!-")
set(VITA_TITLEID  "PLAY00001")
set(VITA_VERSION  "01.00")

add_executable(${PROJECT_NAME}.elf
	main.cpp
	debug_screen.cpp
	font.c
	file_chooser.cpp
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
	./
	../../
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME}.elf
	${PROJECT_LIBS}
	vita2d
	SceAppMgr_stub
	SceDisplay_stub
	SceGxm_stub
	ScePgf_stub
	ScePvf_stub
	SceSysmodule_stub
	SceCommonDialog_stub
	SceCtrl_stub
	-Wl,--whole-archive pthread -Wl,--no-whole-archive
	-lc
	-lstdc++
	-lm
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME}.elf)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
	VERSION ${VITA_VERSION}
	NAME ${VITA_APP_NAME}
)

add_custom_target(launch
	COMMAND curl -T ${PROJECT_NAME}.self ftp://$ENV{PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/eboot.bin
	COMMAND echo "launch ${VITA_TITLEID}" | nc $ENV{PSVITAIP} 1338
	#COMMAND curl "ftp://${PSVITAIP}:1337/ux0:/Play/Play Data Files/logs/GSH_GXM.log"
	DEPENDS ${PROJECT_NAME}.self
)

add_custom_target(send
	COMMAND curl -T ${PROJECT_NAME}.self ftp://$ENV{PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/eboot.bin
	DEPENDS ${PROJECT_NAME}.self
)

add_custom_target(vpksend
	COMMAND curl -T ${PROJECT_NAME}.vpk ftp://$ENV{PSVITAIP}:1337/ux0:/
	DEPENDS ${PROJECT_NAME}.vpk
)
