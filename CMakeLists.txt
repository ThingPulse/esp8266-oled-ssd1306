if (${ESP_PLATFORM})
idf_component_register(SRCS "src/OLEDDisplay.cpp"
                            "src/OLEDDisplayUi.cpp"
                    INCLUDE_DIRS "src")
else()
message("NOT building with esp-idf...")
set(COMPONENT_ADD_INCLUDEDIRS src)
set(COMPONENT_PRIV_REQUIRES arduino-esp32)
set(COMPONENT_SRCDIRS src)
register_component()
endif()