project(raceregistry)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)
set (CMAKE_CXX_STANDARD 17)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

cmake_minimum_required(VERSION 3.20)


# Include OpenSSL libs
find_package(OpenSSL REQUIRED)

# Include PThread libs
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package (SQLite3 REQUIRED)

add_subdirectory(src)

# target_link_libraries(raceRegistryServer
#     PUBLIC
#     Threads::Threads
#     OpenSSL::SSL
# )

find_library(LibRaceSdkCore NAMES libraceSdkCore.so PATHS /usr/local/lib)
if ( NOT LibRaceSdkCore )
        message( WARNING "RaceSdkCore library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "RaceSdkCore library : ${LibRaceSdkCore}")
endif( NOT LibRaceSdkCore )

find_library(LibRaceSdkCommon NAMES libraceSdkCommon.so PATHS /usr/local/lib)
if ( NOT LibRaceSdkCommon )
        message( WARNING "RaceSdkCommon library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "RaceSdkCommon library : ${LibRaceSdkCommon}")
endif( NOT LibRaceSdkCommon )

find_library(LibRaceSdkStorageEncryption NAMES libraceSdkStorageEncryption.so PATHS /usr/local/lib)
if ( NOT LibRaceSdkStorageEncryption )
        message( WARNING "RaceSdkStorageEncryption library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "RaceSdkCore library : ${LibRaceSdkStorageEncryption}")
endif( NOT LibRaceSdkStorageEncryption )

target_link_libraries(raceregistry 
    ${LibRaceSdkCore}
    ${LibRaceSdkCommon}
    ${LibRaceSdkStorageEncryption}
    sqlite3
    Threads::Threads
    OpenSSL::SSL
)

install(TARGETS raceregistry DESTINATION bin)
