# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
<<<<<<< Updated upstream

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(project_name)
=======
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cdc_acm)

include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
>>>>>>> Stashed changes

target_sources(app PRIVATE src/main.c)