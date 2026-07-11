set(COMPONENT_NAME battery_monitor)

if(IDF_TARGET STREQUAL "linux")
    idf_component_register(
        SRCS 
            "src/battery_monitor.cpp"
            "src/adc_battery_reader.cpp"
        INCLUDE_DIRS 
            "include"
            "include/interfaces"
        REQUIRES
            idf_hals
    )
else()
    idf_component_register(
        SRCS 
            "src/battery_monitor.cpp"
            "src/adc_battery_reader.cpp"
        INCLUDE_DIRS 
            "include"
            "include/interfaces"
        REQUIRES 
            idf_hals
    )
endif()

if(IDF_TARGET STREQUAL "linux")
    target_compile_options(${COMPONENT_LIB} PUBLIC --coverage)
    target_link_options(${COMPONENT_LIB} PUBLIC --coverage)
endif()
