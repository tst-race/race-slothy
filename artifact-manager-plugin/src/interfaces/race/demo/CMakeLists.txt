ADD_EXECUTABLE(slothy-race-download
    race-download.cpp
)

target_include_directories(slothy-race-download PRIVATE 
    ${PROJECT_SOURCE_DIR}/include/public
    ${PROJECT_SOURCE_DIR}/include/private
    ${PROJECT_SOURCE_DIR}/external/include
)

target_link_libraries(slothy-race-download
    slothy-race
    crypto
    stdc++fs
    ${LibRaceSdkCore}
    ${LibRaceSdkCommon}
    ${LibRaceSdkStorageEncryption}
    ${LibRaceTestAppShared}
)


install(TARGETS slothy-race-download DESTINATION bin)