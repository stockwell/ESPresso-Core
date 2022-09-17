
file(GLOB_RECURSE SOURCES *.c *.cpp)

set(INCLUDES
        .
        BoilerController
        PumpController
        PressureSensor
        PressureSensor/Sensors
        TemperatureSensor
        TemperatureSensor/Sensors
        Updater
        ADC
        SSR
        TRIAC
        ../Secrets
)

set(REQUIRES
        ESP32-MAX31865
        app_update
        esp_http_client
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
