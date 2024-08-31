target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:DEBUG>:DEBUG>
    $<$<CONFIG:RELEASE>:NDEBUG>
)

# compiler flags as list
set(DEBUG_OPTIONS -Og -g)
set(RELEASE_OPTIONS -O3 -s)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wno-unknown-pragmas
    -fPIE
    $<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>
    $<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>
)
