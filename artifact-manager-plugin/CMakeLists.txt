if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)


cmake_minimum_required(VERSION 3.20)
project(slothy LANGUAGES CXX)
# TODO delete line 
# project(racetestapp-linux LANGUAGES CXX)

set(CMAKE_MODULE_PATH /opt/race/race-cmake-modules)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)

#Static Libraries


find_library(LibJerasure NAMES libJerasure.so) # PATHS /linux/x86_64/lib)
if ( NOT LibJerasure )
        message( WARNING "Dynamic Jerasure library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Dynamic Jerasure library : ${LibJerasure}")
endif( NOT LibJerasure )


# find_library(LibXorcode NAMES libXorcode.a PATHS /opt/dependency-libs/static-libs)
# if ( NOT LibXorcode )
#         message( WARNING "Static Xorcode library NOT FOUND - the respective targets won't be build")
# else()
#         message( STATUS "Static Xorcode library : ${LibXorcode}")
# endif( NOT LibXorcode )


find_library(LibCrypto NAMES libcrypto.so.1.1 PATHS /opt/race-sdk/lib)
if ( NOT LibCrypto )
        message( WARNING "Dynamic Crypto library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Dynamic Crypto library : ${LibCrypto}")
endif( NOT LibCrypto )

find_library(LibErasureCode NAMES liberasurecode.a PATHS /opt/dependency-libs/static-libs)
if ( NOT LibErasureCode )
        message( WARNING "Static ErasureCode library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Static ErasureCode library : ${LibErasureCode}")
endif( NOT LibErasureCode )


find_library(LibGF_Complete NAMES libgf_complete.a PATHS /opt/dependency-libs/static-libs)
if ( NOT LibGF_Complete )
        message( WARNING "Static GF library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Static GF library : ${LibGF_Complete}")
endif( NOT LibGF_Complete )


find_library(LibJson_C NAMES libjson-c.so )
if ( NOT LibJson_C )
        message( WARNING "Dynamic Json-C library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Dynamic Json-C library : ${LibJson_C}")
endif( NOT LibJson_C )

find_library(LibPicoSAT NAMES libpicosat.so PATHS /opt/dependency-libs/dyn-libs)
if ( NOT LibPicoSAT )
        message( WARNING "Dynamic PicoSAT library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Dynamic PicoSAT library : ${LibPicoSAT}")
endif( NOT LibPicoSAT )


find_library(LibRT NAMES librt.a PATHS /opt/dependency-libs/static-libs)
if ( NOT LibRT )
        message( WARNING "Static RT library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Static RT library : ${LibRT}")
endif( NOT LibRT )


find_library(LibSodium NAMES libsodium.so)
if ( NOT LibSodium )
        message( WARNING "Dynamic Sodium library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Dynamic Sodium library : ${LibSodium}")
endif( NOT LibSodium )

find_library(LibSSL NAMES libssl.so.1.1 PATHS /opt/race-sdk/lib/)
if ( NOT LibSSL )
        message( WARNING "Dybamic SSL library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Dybamic SSL library : ${LibSSL}")
endif( NOT LibSSL )

find_library(LibZ NAMES libz.so.1.2.11 PATHS /opt/dependency-libs/dyn-libs)
if ( NOT LibZ )
        message( WARNING "Dynamic Z library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "Dynamic Z library : ${LibZ}")
endif( NOT LibZ )



# RACE Libraries

find_library(LibRaceSdkCore NAMES libraceSdkCore.so PATHS /opt/race-sdk/lib)
if ( NOT LibRaceSdkCore )
        message( WARNING "RaceSdkCore library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "RaceSdkCore library : ${LibRaceSdkCore}")
endif( NOT LibRaceSdkCore )

find_library(LibRaceSdkCommon NAMES libraceSdkCommon.so PATHS /opt/race-sdk/lib)
if ( NOT LibRaceSdkCommon )
        message( WARNING "RaceSdkCommon library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "RaceSdkCommon library : ${LibRaceSdkCommon}")
endif( NOT LibRaceSdkCommon )

find_library(LibRaceSdkStorageEncryption NAMES libraceSdkStorageEncryption.so PATHS /opt/race-sdk/lib)
if ( NOT LibRaceSdkStorageEncryption )
        message( WARNING "RaceSdkStorageEncryption library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "RaceSdkStorageEncryption library : ${LibRaceSdkStorageEncryption}")
endif( NOT LibRaceSdkStorageEncryption )

# # TODO Do I need this?
# find_library(LibRaceTestAppShared NAMES libraceTestAppShared.so PATHS /opt/race-sdk/lib)
# if ( NOT LibRaceTestAppShared )
#         message( WARNING "LibRaceTestAppShared library NOT FOUND - the respective targets won't be build")
# else()
#         message( STATUS "LibRaceTestAppShared library : ${LibRaceTestAppShared}")
# endif( NOT LibRaceTestAppShared )


# Libraries the RACE SDK links against

find_library(LibZip NAMES libzip.so.5 PATHS /opt/race-sdk/lib)
if ( NOT LibZip )
        message( WARNING "LibZip library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "LibZip library : ${LibZip}")
endif( NOT LibZip )


find_library(LibPython3_7 NAMES libpython3.7m.so.1.0 PATHS /opt/race-sdk/lib)
if ( NOT LibPython3_7 )
        message( WARNING "LibPython3_7 library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "LibPython3_7 library : ${LibPython3_7}")
endif( NOT LibPython3_7 )


find_library(LibThrift NAMES libthrift.so.0.12.0) #-libthrift-0.12.0.so PATHS /opt/race-sdk/lib)
if ( NOT LibThrift )
        message( WARNING "LibThrift library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "LibThrift library : ${LibThrift}")
endif( NOT LibThrift )


find_library(LibYaml NAMES libyaml-cpp.so.0.6 PATHS /opt/race-sdk/lib)
if ( NOT LibYaml )
        message( WARNING "LibYaml library NOT FOUND - the respective targets won't be build")
else()
        message( STATUS "LibYaml library : ${LibYaml}")
endif( NOT LibYaml )







add_subdirectory(src)
