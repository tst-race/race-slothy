# Add the source to the shared library

ADD_EXECUTABLE(raceregistry 
    http/httpServer.cpp
    http/ssl.cpp
    http/request.cpp
    util/strlib.cpp
    main.cpp 
    registry.cpp
)

add_subdirectory(db)
add_subdirectory(race)



# Add headers for this project
target_include_directories(raceregistry
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)



