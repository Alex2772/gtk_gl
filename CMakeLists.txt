cmake_minimum_required(VERSION 3.29)
project(gtk_sample_opengl)

set(CMAKE_CXX_STANDARD 20)

add_executable(gtk_sample_opengl src/main.cpp)
target_include_directories(gtk_sample_opengl PUBLIC src glm)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(adwaita REQUIRED IMPORTED_TARGET libadwaita-1)
#target_link_libraries(gtk_sample_opengl PRIVATE PkgConfig::GTK4 PkgConfig::adwaita epoxy)
target_link_libraries(gtk_sample_opengl PRIVATE /home/alex2772/Projects/gtk/buildDir/gtk/libgtk-4.so PkgConfig::adwaita epoxy)
