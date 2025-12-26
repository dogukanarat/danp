# ============================================================================
# Example Applications
# ============================================================================

# Helper function to create example executables
function(danp_add_example EXAMPLE_NAME)
    cmake_parse_arguments(
        ARG
        ""
        "SOURCE"
        "ADDITIONAL_SOURCES"
        ${ARGN}
    )

    # Create executable
    add_executable(${EXAMPLE_NAME}
        ${ARG_SOURCE}
        ${ARG_ADDITIONAL_SOURCES}
        example_log_message.c
    )

    # Link against library
    target_link_libraries(${EXAMPLE_NAME}
        PRIVATE
            danp::danp
    )

    # Set output name
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        OUTPUT_NAME ${EXAMPLE_NAME}
    )
endfunction()

# ============================================================================
# DGRAM Examples
# ============================================================================
danp_add_example(danp_dgram_client SOURCE dgram/client.c)
danp_add_example(danp_dgram_server SOURCE dgram/server.c)

# ============================================================================
# STREAM Examples
# ============================================================================
danp_add_example(danp_stream_client SOURCE stream/client.c)
danp_add_example(danp_stream_server SOURCE stream/server.c)

# ============================================================================
# Zero-Copy Examples
# ============================================================================
danp_add_example(danp_zerocopy_client SOURCE zerocopy/client.c)
danp_add_example(danp_zerocopy_server SOURCE zerocopy/server.c)
danp_add_example(danp_sfp_example SOURCE zerocopy/sfp_example.c)

# ============================================================================
# Example Summary
# ============================================================================
message(STATUS "Example applications configured:")
message(STATUS "  DGRAM:")
message(STATUS "    - danp_dgram_client")
message(STATUS "    - danp_dgram_server")
message(STATUS "  STREAM:")
message(STATUS "    - danp_stream_client")
message(STATUS "    - danp_stream_server")
message(STATUS "  Zero-Copy:")
message(STATUS "    - danp_zerocopy_client")
message(STATUS "    - danp_zerocopy_server")
message(STATUS "    - danp_sfp_example")
