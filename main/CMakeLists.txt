
file(GLOB_RECURSE SOURCES *.c *.cpp)

set(INCLUDES
        .
        BoilerController
        PressureSensor
        PressureSensor/Sensors
        TemperatureSensor
        TemperatureSensor/Sensors
        ADC
)

set(REQUIRES
        ESP32-MAX31865
        esp_http_server
        esp_wifi
        json
        mdns
        nvs_flash
        QuickPID
)

idf_component_register(SRCS         ${SOURCES}
                       INCLUDE_DIRS ${INCLUDES}
                       REQUIRES     ${REQUIRES})
