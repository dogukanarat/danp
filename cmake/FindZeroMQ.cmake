find_package(PkgConfig)
pkg_check_modules(PKGZMQ libzmq)

find_path(Find_ZeroMQ_INCLUDE_DIRS zmq.h
          PATHS ${ZeroMQ_INCLUDE_DIRS_HINT}
                ${PKGZMQ_INCLUDE_DIRS}
                /usr/include
                /usr/local/include
                /usr/include/zmq
                /usr/local/include/zmq)

find_library(Find_ZeroMQ_LIBRARIES NAMES zmq
             PATHS ${ZeroMQ_LIBRARIES_HINT}
                   ${PKGZMQ_LIBRARY_DIRS}
                   /usr/lib
                   /usr/local/lib
                   /usr/lib/zmq
                   /usr/local/lib/zmq)

if(Find_ZeroMQ_LIBRARIES AND Find_ZeroMQ_INCLUDE_DIRS)
    set(ZeroMQ_FOUND ON)
    #message(STATUS "Found ZeroMQ")
    #message(STATUS "ZeroMQ libraries: ${Find_ZeroMQ_LIBRARIES}")
    #message(STATUS "ZeroMQ include directories: ${Find_ZeroMQ_INCLUDE_DIRS}")
endif()

set(ZeroMQ_LIBRARIES ${Find_ZeroMQ_LIBRARIES})
set(ZeroMQ_INCLUDE_DIRS ${Find_ZeroMQ_INCLUDE_DIRS})

if(NOT TARGET zmq)
    add_library(zmq UNKNOWN IMPORTED)
    set_target_properties(zmq PROPERTIES
        IMPORTED_LOCATION ${ZeroMQ_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${ZeroMQ_INCLUDE_DIRS})
endif()
