cmake_minimum_required(VERSION 1.0)
project(SD3403OBJ)

set(OBJ_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SS928_SDK "/home/ebaina/ss928v100_sdk/SS928V100_SDK_V2.0.2.1")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O2")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CROSS_C_COMPILE aarch64-mix210-linux-gcc)
set(CROSS_CXX_COMPILE aarch64-mix210-linux-g++)
set(CMAKE_C_COMPILER ${CROSS_C_COMPILE})
set(CMAKE_CXX_COMPILER ${CROSS_CXX_COMPILE})


include_directories(
    "${SS928_SDK}/smp/a55_linux/mpp/out/include"
    "${SS928_SDK}/smp/a55_linux/mpp/out/include/svp_npu"
    "${OBJ_SOURCE}/source"
    "${OBJ_SOURCE}/sys"
    "${OBJ_SOURCE}/vi"
    "${OBJ_SOURCE}/venc"
    "${OBJ_SOURCE}/rgn"
    "${OBJ_SOURCE}/vpss"
    "${OBJ_SOURCE}/manage"
    "${OBJ_SOURCE}/common"
    "${OBJ_SOURCE}/fifo_buffer"
    "${OBJ_SOURCE}/svp_npu/camera_model_det"
    "${OBJ_SOURCE}/svp_npu/camera_track_obj"
    "${OBJ_SOURCE}/3rdLibrary/rtsp/include"
    "${OBJ_SOURCE}/3rdLibrary/rtsp/live555/include/BasicUsageEnvironment"
    "${OBJ_SOURCE}/3rdLibrary/rtsp/live555/include/groupsock"
    "${OBJ_SOURCE}/3rdLibrary/rtsp/live555/include/liveMedia"
    "${OBJ_SOURCE}/3rdLibrary/rtsp/live555/include/UsageEnvironment"
    "${OBJ_SOURCE}/3rdLibrary/openssl/include/"
    "${OBJ_SOURCE}/3rdLibrary/opencv/include"
    "${OBJ_SOURCE}/rtsp/live555"
)
link_directories(
    "${SS928_SDK}/smp/a55_linux/mpp/out/lib"
    "${SS928_SDK}/smp/a55_linux/mpp/out/lib/svp_npu"
    "${OBJ_SOURCE}/3rdLibrary/rtsp/lib"
    "${OBJ_SOURCE}/3rdLibrary/rtsp/live555/lib"
    "${OBJ_SOURCE}/3rdLibrary/openssl/lib"
    "${OBJ_SOURCE}/3rdLibrary/opencv/lib"
)
message(STATUS  "${OBJ_SOURCE}/3rdLibrary/opencv/lib")

file(GLOB SRC
    "${OBJ_SOURCE}/sys/sd3403_sys.cpp"
    "${OBJ_SOURCE}/venc/sd3403_venc.cpp"
    "${OBJ_SOURCE}/vi/sd3403_vi.cpp"
    "${OBJ_SOURCE}/vpss/sd3403_vpss.cpp"
    "${OBJ_SOURCE}/rgn/sd3403_rgn.cpp"
    "${OBJ_SOURCE}/source/sd3403_main.cpp"
    "${OBJ_SOURCE}/manage/sd3403_manage.cpp"
    "${OBJ_SOURCE}/common/sd3403_common.cpp"
    "${OBJ_SOURCE}/fifo_buffer/Fifo_Buffer.cpp"
    "${OBJ_SOURCE}/fifo_buffer/Pthread_Lock.cpp"
    "${OBJ_SOURCE}/svp_npu/camera_model_det/svp_npu.cpp"
    "${OBJ_SOURCE}/svp_npu/camera_track_obj/*.cpp"
    "${OBJ_SOURCE}/rtsp/live555/GetFrame.cpp"
    "${OBJ_SOURCE}/rtsp/live555/H264FramedLiveSource.cpp"
    "${OBJ_SOURCE}/rtsp/live555/H264LiveVideoServerMediaSubssion.cpp"
    "${OBJ_SOURCE}/rtsp/live555/Rtsp_Server.cpp"
    # "${OBJ_SOURCE}/svp_npu/camera_track_obj/Hungarian.cpp"
    # "${OBJ_SOURCE}/svp_npu/camera_track_obj/KalmanTracker.cpp"
    # "${OBJ_SOURCE}/svp_npu/camera_track_obj/pch.cpp"
)

add_subdirectory(examples/model)
add_subdirectory(examples/decode)

add_executable(sd3403_obj ${SRC})
target_compile_definitions(sd3403_obj PRIVATE RTSP_LIVE555_ON)
set(live555_lib liveMedia groupsock BasicUsageEnvironment UsageEnvironment)
set(openssl_lib ssl crypto)
set(sd3403_lib mpi isp ot_isp securec _hiawb _hiae sns_imx485 sns_imx347_slave sns_os05a10_2l_slave sns_os08b10 sns_os04a10 sns_os08a20 VoiceEngine 
    upvqe _hiacs dnvqe _hildci _hidrc _hidehaze _hiir_auto _hibnr _hiextend_stats _hicalcflicker _hicrb svp_acl protobuf-c)
file(GLOB OPENCV_LIBS "${OBJ_SOURCE}/3rdLibrary/opencv/lib/*.so")
# set(OPENCV_LIB opencv_core opencv_imgproc opencv_imgcodecs opencv_video jpeg png16 tiff z jbig)
target_link_libraries(sd3403_obj pthread ${sd3403_lib} rtsp ${OPENCV_LIBS} ${live555_lib} ${openssl_lib})

# if(DEFINED EXAMPLE)
#     if(${EXAMPLE} STREQUAL "all")
#     else if(${EXAMPLE} STREQUAL "model")
#         add_executable(model_test ${SRC})
#     endif()

# endif()