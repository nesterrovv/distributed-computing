add_executable(pa5
        ${CMAKE_CURRENT_SOURCE_DIR}/pa2345.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lamport_time.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cs.c
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    set(RUNTIME_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib64 CACHE PATH "Runtime library path")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
    set(RUNTIME_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib32 CACHE PATH "Runtime library path")
endif()
find_library(RUNTIME_LIBRARY libruntime.so PATHS ${RUNTIME_LIBRARY_DIR} NO_DEFAULT_PATH)
if(RUNTIME_LIBRARY)
    message(STATUS "Runtime library was found: ${RUNTIME_LIBRARY}")
else(RUNTIME_LIBRARY)
    message(FATAL_ERROR "Cannot find runtime library (searched ${RUNTIME_LIBRARY_DIR})")
endif(RUNTIME_LIBRARY)
set_property(TARGET pa5
        PROPERTY
        ENVIRONMENT LD_PRELOAD="${RUNTIME_LIBRARY}")
target_link_libraries(pa5 ${RUNTIME_LIBRARY})