cmake_minimum_required(VERSION 3.5)

project(Motor_Grafico_Belkan )

# problema deteccion librerias glut - mgo -------
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/usr/lib/x86_64-linux-gnu -lglut")
set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/libglut")

#########################################################
# FIND GLUT
#########################################################
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
link_directories(${GLUT_LIBRARY_DIRS})
add_definitions(${GLUT_DEFINITIONS})
if(NOT GLUT_FOUND)
    message(ERROR " GLUT not found!")
endif(NOT GLUT_FOUND)

#########################################################
# FIND OPENGL
#########################################################
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)

#########################################################
# FIND OPENMP
#########################################################
find_package(OpenMP)
if(OPENMP_FOUND)
    include_directories(${OpenMP_INCLUDE_DIRS})
    link_directories(${OpenMP_LIBRARY_DIRS})
    add_definitions(${OpenMP_DEFINITIONS})
else()
    message(WARNING "OpenMP not found, building without OpenMP support")
endif()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

#########################################################
# FIND LIBJPEG
#########################################################
find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIRS})
link_directories(${JPEG_LIBRARY_DIRS})
add_definitions(${JPEG_DEFINITIONS})
if(NOT JPEG_FOUND)
    message(ERROR " JPEG not found!")
endif(NOT JPEG_FOUND)


#########################################################
# Source code...
#########################################################
file(GLOB SOURCES "src/*.cpp" "src/*.cxx" "src/*.cc")
file(GLOB SOURCES_LIB3D "src/obj3dlib/*.cpp" "src/obj3dlib/*.cxx" "src/obj3dlib/*.cc" "src/obj3dlib/*/*.cpp" "src/obj3dlib/*/*.cxx" "src/obj3dlib/*/*.cc")
file(GLOB SOURCES_LIBMOTOR "src/motorlib/*.cpp" "src/motorlib/*.cxx" "src/motorlib/*.cc")
file(GLOB SOURCES_MODELOS "src/modelos/*/*.cpp" "src/modelos/*/*.cxx" "src/modelos/*.cc" "src/modelos/*.cpp" "src/modelos/*.cxx" "src/modelos/*.cc")
file(GLOB SOURCES_GLUI "src/glui/*.cpp" "src/glui/*.cxx" "src/glui/*.cc")
file(GLOB SOURCES_COMPORTAMIENTOS 
    "src/comportamientos/*.cpp" 
    "Comportamientos_Agentes/*.cpp" 
    "src/comportamientos/npcs/*.cpp"
)
# Marcar include/third_party_system (que apunta a glui) como directorio de sistema 
# para suprimir warnings en glui.h en todo el proyecto.
include_directories(SYSTEM "include/third_party_system")
include_directories("include")
include_directories(".")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

########################################################
# Compiling & stuff
#########################################################
add_library(obj3DLib ${SOURCES_LIB3D})
target_link_libraries(obj3DLib ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${JPEG_LIBRARIES} "-lpthread")

add_library(GLUILib ${SOURCES_GLUI})
target_link_libraries(GLUILib ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
# Suprimir todos los warnings en la biblioteca GLUI (código de terceros)
target_compile_options(GLUILib PRIVATE -w)
target_include_directories(GLUILib SYSTEM PRIVATE "include/glui")

add_library(modelosBelkanLib ${SOURCES_MODELOS})
if(OPENMP_FOUND)
    target_link_libraries(modelosBelkanLib obj3DLib ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${OPENMP_LIBRARIES} "-lpthread")
else()
    target_link_libraries(modelosBelkanLib obj3DLib ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} "-lpthread")
endif()

add_library(comportamientosLib ${SOURCES_COMPORTAMIENTOS})
if(OPENMP_FOUND)
    target_link_libraries(comportamientosLib ${OPENMP_LIBRARIES} "-lpthread")
else()
    target_link_libraries(comportamientosLib "-lpthread")
endif()

add_library(motorLib ${SOURCES_LIBMOTOR})
if(OPENMP_FOUND)
    target_link_libraries(motorLib obj3DLib GLUILib comportamientosLib OpenGL::GL GLUT::GLUT ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${OPENMP_LIBRARIES} "-lpthread")
else()
    target_link_libraries(motorLib obj3DLib GLUILib comportamientosLib OpenGL::GL GLUT::GLUT ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} "-lpthread")
endif()

add_executable(practica2 "bin_src/juego.cc" ${SOURCES})
if(OPENMP_FOUND)
    target_link_libraries(practica2 obj3DLib motorLib modelosBelkanLib ${OPENMP_LIBRARIES} "-lpthread")
else()
    target_link_libraries(practica2 obj3DLib motorLib modelosBelkanLib "-lpthread")
endif()

add_executable(practica2SG "bin_src/juegoSG.cc" ${SOURCES})
if(OPENMP_FOUND)
    target_link_libraries(practica2SG   obj3DLib motorLib modelosBelkanLib ${OPENMP_LIBRARIES} "-lpthread")
else()
    target_link_libraries(practica2SG   obj3DLib motorLib modelosBelkanLib "-lpthread")
endif()
