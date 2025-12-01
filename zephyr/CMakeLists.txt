if(CONFIG_DANP)
    zephyr_library()

    # Add your DANP source files
    zephyr_library_sources(
        ../src/danp.c
        ../src/danpSocket.c
        # Add any other source files from src/ here
    )

    # Include directories
    zephyr_include_directories(
        ../include
        ../src
    )

    # Link against the OSAL library
    zephyr_library_link_libraries(osal)
endif()
