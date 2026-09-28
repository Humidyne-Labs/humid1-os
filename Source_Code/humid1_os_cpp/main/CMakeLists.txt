idf_component_register(
    SRCS 
        "main.cpp"
        "app_state.cpp"
        "system/button_handler.cpp"
        "telemetry/sensor_reader.cpp"
        "network/prov_manager.cpp"
        "cloud/tb_client.cpp"
        "ui/ui_manager.cpp"
        "ui/screen_provisioning.cpp"
        "ui/screen_claiming.cpp"
        "ui/screen_dashboard.cpp"
        "ui/screen_ota.cpp"
    INCLUDE_DIRS 
        "."
        "system"
        "telemetry"
        "network"
        "cloud"
        "ui"
    PRIV_REQUIRES 
        esp32_s3_bsp 
        esp_netif
        esp_event
        esp_wifi
        bt
        nvs_flash
        app_update
        esp_https_ota
        esp_http_client
        cjson
        mqtt
        network_provisioning
        lvgl
)

# Pack assets and specify file extension(s)
spiffs_create_partition_assets(storage ../spiflash_assets
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".png"
    MMAP_SUPPORT_RAW
    MMAP_RAW_COLOR_FORMAT "I1"
)