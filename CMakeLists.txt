cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

project(MediaImporter)
set(PROJECT_TITLE "Media Importer")
set(TITLE_ID "MIMP00001")

set(CMAKE_C_FLAGS "-Wl,-q -O3 -std=c99")

add_definitions(-DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0)

include_directories(
	sqlite-3.6.23.1
)

add_executable(${PROJECT_NAME}.elf
	src/main.c
	src/debugScreen.c
	src/debugScreenFont.c
	src/vita_sqlite.c
	src/video.c
	src/music.c
	src/mp4utils.c
	src/id3.c
	sqlite-3.6.23.1/sqlite3.c
)

target_link_libraries(${PROJECT_NAME}.elf
	-lSceDisplay_stub
	-lSceCtrl_stub
	-lSceSysmodule_stub
	-lSceAppUtil_stub
)

add_custom_target(${PROJECT_NAME}.vpk ALL
	COMMAND vita-elf-create ${PROJECT_NAME}.elf ${PROJECT_NAME}.velf
	COMMAND vita-make-fself ${PROJECT_NAME}.velf eboot.bin
	COMMAND vita-mksfoex -s TITLE_ID=${TITLE_ID} "${PROJECT_TITLE}" param.sfo
	COMMAND vita-pack-vpk -s param.sfo -b eboot.bin
		-a ${PROJECT_SOURCE_DIR}/sce_sys/icon0.png=sce_sys/icon0.png
		-a ${PROJECT_SOURCE_DIR}/sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png
		-a ${PROJECT_SOURCE_DIR}/sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png
		-a ${PROJECT_SOURCE_DIR}/sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml
		${PROJECT_NAME}.vpk
)

add_custom_target(vpksend
	COMMAND curl -T ${PROJECT_NAME}.vpk ftp://$(PSVITAIP):1337/ur0:/vpks/
	DEPENDS ${PROJECT_NAME}.vpk
)

add_custom_target(send
	COMMAND curl -T eboot.bin ftp://$(PSVITAIP):1337/ux0:/app/${TITLE_ID}/
	DEPENDS ${PROJECT_NAME}.vpk
)

add_dependencies(${PROJECT_NAME}.vpk ${PROJECT_NAME}.elf)
