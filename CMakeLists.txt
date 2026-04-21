# Ungula Display Library
# Copyright © 2026, Alex Conesa
# MIT License

cmake_minimum_required(VERSION 3.15)

if(ESP_PLATFORM)
	# Build UngulaDisplay as an ESP-IDF component
	idf_component_register(INCLUDE_DIRS src)
	return()
endif()

project(UngulaDisplay VERSION 1.0.2)

add_subdirectory(src)
