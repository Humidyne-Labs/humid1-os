idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_mmap_assets esp_lv_fs)

# Pack assets and specify file extension(s)
spiffs_create_partition_assets(storage ../spiflash_assets
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".png"
    MMAP_SUPPORT_RAW
    MMAP_RAW_COLOR_FORMAT "I1" # Options: RGB565, RGB888, ARGB8888, AUTO, etc.
)