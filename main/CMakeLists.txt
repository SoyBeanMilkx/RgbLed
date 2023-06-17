# 查找 test_path 目录下的所有源文件，保存到 TEST_SRCS 变量中
file(GLOB COLORUTILS "color/*.c")

idf_component_register(SRCS "rgb_led.c" ${COLORUTILS}
        INCLUDE_DIRS "")
