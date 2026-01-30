# copy_if_not_exists.cmake
# 如果目标文件不存在，则从源文件复制
# 用法: cmake -DSRC_FILE=<source> -DDST_FILE=<destination> -P copy_if_not_exists.cmake

if(NOT EXISTS "${DST_FILE}")
    # 确保目标目录存在
    get_filename_component(DST_DIR "${DST_FILE}" DIRECTORY)
    if(NOT EXISTS "${DST_DIR}")
        file(MAKE_DIRECTORY "${DST_DIR}")
    endif()

    # 复制文件
    file(COPY "${SRC_FILE}" DESTINATION "${DST_DIR}")
    get_filename_component(SRC_NAME "${SRC_FILE}" NAME)
    get_filename_component(DST_NAME "${DST_FILE}" NAME)

    # 如果源文件名和目标文件名不同，重命名
    if(NOT "${SRC_NAME}" STREQUAL "${DST_NAME}")
        file(RENAME "${DST_DIR}/${SRC_NAME}" "${DST_FILE}")
    endif()

    message(STATUS "Copied ${SRC_FILE} to ${DST_FILE}")
else()
    message(STATUS "Skipped copying to ${DST_FILE} (file exists, preserving user modifications)")
endif()
