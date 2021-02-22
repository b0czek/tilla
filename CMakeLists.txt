cmake_minimum_required(VERSION 3.5)

# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
set(EXTRA_COMPONENT_DIRS components/esp-network)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(restful_server)
