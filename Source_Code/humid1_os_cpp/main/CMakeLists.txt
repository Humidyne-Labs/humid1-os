idf_component_register(
    SRCS 
        "main.cpp"
    INCLUDE_DIRS 
        "."
    PRIV_REQUIRES 
        esp32-s3_bsp 
        esp_netif
        esp_event
        esp_wifi
        bt
        nvs_flash
        app_update
        esp_https_ota
        esp_http_client
)

# Pack assets and specify file extension(s)
spiffs_create_partition_assets(storage ../spiflash_assets
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".png"
    MMAP_SUPPORT_RAW
    MMAP_RAW_COLOR_FORMAT "I1"
)