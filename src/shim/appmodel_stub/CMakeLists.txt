# Side-by-side AppModel runtime stub for legacy Windows (7/8/8.1, VxKex).
# Built only on WIN32; produces api-ms-win-appmodel-runtime-l1-1-2.dll which
# the OS loader prefers app-locally over the missing system contract.
add_library(appmodel_stub SHARED appmodel_stub.cc appmodel_stub.def)

set_target_properties(appmodel_stub PROPERTIES
    PREFIX ""
    OUTPUT_NAME "api-ms-win-appmodel-runtime-l1-1-2"
    SUFFIX ".dll"
    NO_EXPORT TRUE
)

target_link_libraries(appmodel_stub PRIVATE kernel32)
