file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(circle-quantizer "${SOURCES}")
target_include_directories(circle-quantizer PRIVATE include)
target_include_directories(circle-quantizer PRIVATE src)
target_link_libraries(circle-quantizer foder)
target_link_libraries(circle-quantizer safemain)
target_link_libraries(circle-quantizer oops)
target_link_libraries(circle-quantizer loco)
target_link_libraries(circle-quantizer mio_circle)
target_link_libraries(circle-quantizer luci_import)
target_link_libraries(circle-quantizer luci_service)
target_link_libraries(circle-quantizer luci_pass)
target_link_libraries(circle-quantizer luci_export)
target_link_libraries(circle-quantizer arser)
target_link_libraries(circle-quantizer vconone)

install(TARGETS circle-quantizer DESTINATION bin)
