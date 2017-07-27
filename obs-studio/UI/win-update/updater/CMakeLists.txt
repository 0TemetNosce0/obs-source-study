if(NOT ENABLE_WIN_UPDATER)
	return()
endif()

if(DISABLE_UPDATE_MODULE)
	return()
endif()

if(NOT DEFINED STATIC_ZLIB_PATH OR "${STATIC_ZLIB_PATH}" STREQUAL "")
	message(STATUS "STATIC_ZLIB_PATH not set, windows updater disabled")
	return()
endif()

project(updater)

include_directories(${OBS_JANSSON_INCLUDE_DIRS})
include_directories(${LIBLZMA_INCLUDE_DIRS})
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")
include_directories(${BLAKE2_INCLUDE_DIR})

find_package(ZLIB REQUIRED)

set(updater_HEADERS
	../win-update-helpers.hpp
	resource.h
	updater.hpp
	)
set(updater_SOURCES
	../win-update-helpers.cpp
	updater.cpp
	patch.cpp
	http.cpp
	hash.cpp
	updater.rc
	)

add_definitions(-DNOMINMAX -DUNICODE -D_UNICODE)
if(MSVC)
	add_compile_options("$<$<CONFIG:RelWithDebInfo>:/MT>")
        #add_compile_options的参数可以使用带语法$<...>的“生成表达式”。关于有效的表达式可以查看cmake-generator-expressions(7)手册
        #编译选项可以通过add_compile_options命令，也可以通过set命令修改CMAKE_CXX_FLAGS或CMAKE_C_FLAGS。
        #使用这两种方式在有的情况下效果是一样的，但请注意它们还是有区别的：
        #add_compile_options命令添加的编译选项是针对所有编译器的(包括c和c++编译器)，而set命令设置CMAKE_C_FLAGS或CMAKE_CXX_FLAGS变量则是分别只针对c和c++编译器的。
endif()

add_executable(updater WIN32
	${updater_HEADERS}
	${updater_SOURCES}
	)
target_link_libraries(updater
	${OBS_JANSSON_IMPORT}
	${STATIC_ZLIB_PATH}
	lzma
	blake2
	psapi
	comctl32
	shell32
	winhttp
	)
