function(target_embed_images target images)
    foreach(image ${images})
        execute_process(
            COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/png2c/png2c.py" "${CMAKE_CURRENT_SOURCE_DIR}/${image}"
            OUTPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/img/${image}.h"
        )
        target_sources(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/img/${image}.h)
    endforeach()
endfunction()