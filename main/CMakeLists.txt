# /main mappán belül megadható mappában lévő
# összes .c és .h fájlokat keres és mindet
# bele fogja buildelni a projektbe
set(ALL_PROJECT_DIRS "./app" "./bluetooth" "./codec" "./hotspot")

idf_component_register(SRC_DIRS ${ALL_PROJECT_DIRS}
                        INCLUDE_DIRS ${ALL_PROJECT_DIRS})

# /main mappán belül megadott mappából feltölt
# mindent az SPI flash-nek a filesystemjére (SPIFFS)
# spiffs_create_partition_image(storage ./spiffs_files FLASH_IN_PROJECT)
