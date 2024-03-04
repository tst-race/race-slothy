# libsss.so
# Core sharding code, slothy depends on this, but 
# the shard-sharding server only needs this library to function
ADD_LIBRARY(sss SHARED 
    shard.cpp
    util/rs_shard.cpp
    util/crypto_secretbox.cpp
    util/sss.c
    util/hazmat.c
    util/file_descriptor.cpp
    util/concat.cpp
    util/kernel.cpp
    util/hex.cpp
    util/logger.cpp
    util/profiler.cpp
)

target_include_directories(sss PRIVATE 
    ${PROJECT_SOURCE_DIR}/include/public
    ${PROJECT_SOURCE_DIR}/include/private
    ${PROJECT_SOURCE_DIR}/external/include
)

target_link_libraries(sss
    ${LibRT}
    ${LibJerasure}
    ${LibSSL}
)


# libslothy.so
# Core slothy library, containing helper functions and base objects
# for the extension interfaces to build on
ADD_LIBRARY(slothy SHARED
    slothy.cpp
    shard.cpp
    download.cpp
    util/sleep.cpp
    util/base64.cpp
)

# set_target_properties(slothy PROPERTIES INSTALL_RPATH "$ORIGIN")

target_link_libraries(slothy
    sss

    ${LibJerasure}
    ${LibXorcode}
    ${LibCrypto}
    -lcurl
    ${LibErasureCode}
    ${LibGF_Complete}
    ${LibJson_C}
    ${LibPicoSAT}
    ${LibRT}
    ${LibSodium}
    ${LibSSL}
    ${LibZ}

    ${LibZip}
    ${LibPython3_7}
    ${LibThrift}
    ${LibYaml}
)

target_include_directories(slothy PRIVATE 
    ${PROJECT_SOURCE_DIR}/include/public
    ${PROJECT_SOURCE_DIR}/include/private
    ${PROJECT_SOURCE_DIR}/external/include
)



# # libslothy-race.so
# # Shared object for standalone RACE clients to download arbitrary
# # artifacts
# ADD_LIBRARY(slothy-race SHARED
#     interfaces/race/slothy-race.cpp
#     interfaces/race/Message.cpp
#     interfaces/race/RaceApp.cpp 
#     interfaces/race/RaceInterface.cpp 
#     interfaces/race/RaceUtil.cpp 
#     interfaces/race/ReceivedMessage.cpp
#     interfaces/race/UserInputResponseCache.cpp 
#     interfaces/race/UserInputResponseParser.cpp 
# )

# target_include_directories(slothy-race PRIVATE 
#     ${PROJECT_SOURCE_DIR}/include/public
#     ${PROJECT_SOURCE_DIR}/include/private
#     ${PROJECT_SOURCE_DIR}/include/private/interfaces/race
#     ${PROJECT_SOURCE_DIR}/external/include
# )

# target_link_libraries(slothy-race
#     slothy
# )



# # Desend into the demo directory for the standalone RACE demo client
# add_subdirectory(interfaces/http/demo)

# libslothy-amp.so
# Shared object for use as an AMP plugin for bootstrapping RACE nodes
ADD_LIBRARY(slothy-amp SHARED
    interfaces/amp/slothy-amp.cpp
)

# set_target_properties(slothy-amp PROPERTIES INSTALL_RPATH "$ORIGIN")

target_include_directories(slothy-amp PRIVATE 
    ${PROJECT_SOURCE_DIR}/include/public
    ${PROJECT_SOURCE_DIR}/include/private
    ${PROJECT_SOURCE_DIR}/include/private/interfaces/amp
    ${PROJECT_SOURCE_DIR}/external/include
)

target_link_libraries(slothy-amp
    slothy

    ${LibRaceSdkCore}
    ${LibRaceSdkCommon}
    ${LibRaceSdkStorageEncryption}
)

set_target_properties(slothy-amp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    BUILD_RPATH "\${ORIGIN}/"
    INSTALL_RPATH "\${ORIGIN}/"
)

ADD_LIBRARY(slothy-http SHARED
    interfaces/http/slothy-http.cpp
)

target_include_directories(slothy-http PRIVATE 
    ${PROJECT_SOURCE_DIR}/include/public
    ${PROJECT_SOURCE_DIR}/include/private
    ${PROJECT_SOURCE_DIR}/include/private/interfaces/http
    ${PROJECT_SOURCE_DIR}/external/include
)

target_link_libraries(slothy-http
    slothy
)



install(TARGETS sss DESTINATION lib)
install(TARGETS slothy DESTINATION lib)
install(TARGETS slothy-http DESTINATION lib)
install(TARGETS slothy-amp DESTINATION lib)
# install(TARGETS slothy-race DESTINATION lib)
