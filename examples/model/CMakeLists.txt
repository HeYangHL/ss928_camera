# cmake_minimum_required(VERSION 1.0)
# project(MODEL)


file(GLOB MOD_SRC
    "${OBJ_SOURCE}/svp_npu/camera_model_det/svp_npu.cpp"
    "${OBJ_SOURCE}/examples/model/obj_detection.cpp"
)

link_directories(
    "${OBJ_SOURCE}/3rdLibrary/opencv/lib"
)

include_directories(
    "${OBJ_SOURCE}/examples/model"
    "${OBJ_SOURCE}/3rdLibrary/opencv/include"
)
message(STATUS  "${OBJ_SOURCE}/3rdLibrary/opencv/lib")

add_executable(obj_detec ${MOD_SRC})
set(svp_lib svp_acl protobuf-c)
set(opencv_lib opencv_core opencv_imgproc opencv_imgcodecs jpeg png16 tiff z jbig)
set(ss928_lib mpi isp ot_isp securec _hiawb _hiae sns_imx485 sns_imx347_slave sns_os05a10_2l_slave sns_os08b10 sns_os04a10 sns_os08a20 VoiceEngine 
upvqe _hiacs dnvqe _hildci _hidrc _hidehaze _hiir_auto _hibnr _hiextend_stats _hicalcflicker _hicrb)
target_link_libraries(obj_detec pthread ${svp_lib} ${opencv_lib} ${ss928_lib})