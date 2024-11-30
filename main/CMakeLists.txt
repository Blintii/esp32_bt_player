# /main mappán belül megadható mappában lévő
# összes .c és .h fájlokat keres és mindet
# bele fogja buildelni a projektbe
set(ALL_PROJECT_DIRS "./app" "./bluetooth" "./codec" "./light" "./hotspot")

string(REGEX MATCH ".*/main$" is_main_dir "${CMAKE_CURRENT_SOURCE_DIR}")

if(is_main_dir)
    execute_process(
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${python} ${CMAKE_CURRENT_SOURCE_DIR}/dsp_lut_gen.py --fft_exp 11 app/dsp_fft_lut.c
        RESULT_VARIABLE result
    )

    if(result EQUAL 0)
        message("dsp gen OK")
    else()
        message(FATAL_ERROR "dsp gen failed")
    endif()
endif()

idf_component_register(
    SRC_DIRS ${ALL_PROJECT_DIRS}
    INCLUDE_DIRS ${ALL_PROJECT_DIRS}
)

# /main mappán belül megadott mappából feltölt
# mindent az SPI flash-nek a filesystemjére (SPIFFS)
# spiffs_create_partition_image(storage ./spiffs_files FLASH_IN_PROJECT)
