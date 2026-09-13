# cmake_minimum_required(VERSION 3.10)
# project(SD3403_DECODE)

file(GLOB DEC_SRC
    "${OBJ_SOURCE}/examples/decode/decode.cpp"
)

include_directories(
    "${OBJ_SOURCE}/examples/decode"
    "${SS928_SDK_PATH}/smp/a55_linux/mpp/out/include"
)

add_executable(sd3403_decode ${DEC_SRC})

set(ss928_lib mpi isp ot_isp securec _hiawb _hiae sns_imx485 sns_imx347_slave sns_os05a10_2l_slave sns_os08b10 sns_os04a10 sns_os08a20 VoiceEngine 
upvqe _hiacs dnvqe _hildci _hidrc _hidehaze _hiir_auto _hibnr _hiextend_stats _hicalcflicker _hicrb)
target_link_libraries(sd3403_decode pthread ${ss928_lib})
