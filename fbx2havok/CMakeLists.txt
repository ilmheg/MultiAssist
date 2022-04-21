cmake_minimum_required(VERSION 3.10)
project(fbx2havok)

set(
    PROJ_LIBRARIES
        hkBase.lib
		hkSerialize.lib
		hkSceneData.lib
		hkVisualize.lib
		hkInternal.lib
		hkImageUtilities.lib
		hkaAnimation.lib
		hkaInternal.lib
		hkaPhysics2012Bridge.lib
		hkcdCollide.lib
		hkcdInternal.lib
		hkGeometryUtilities.lib
		hkCompat.lib
		libfbxsdk-md.lib
)

include_directories(
    "${FBX_SDK_ROOT}/include/"
    "${HAVOK_SDK_ROOT}/Source/"
)

link_directories(
    "${FBX_SDK_ROOT}/lib/vs2012/x86/debug/"
    "${HAVOK_SDK_ROOT}/Lib/win32_vs2012_win7_noSimd/debug"
)

add_library(commonLib
		Core/EulerAngles.h
		Core/FBXCommon.cxx
		Core/FBXCommon.h
		Core/MathHelper.h
		Core/stdafx.cpp
		Core/stdafx.h
		Core/targetver.h
)

add_executable(fbx2havok Core/main.cpp)
target_link_libraries(fbx2havok ${PROJ_LIBRARIES} commonLib)

add_executable(tofbx Core/tofbx.cpp)
target_link_libraries(tofbx ${PROJ_LIBRARIES} commonLib)

set(CMAKE_CXX_STANDARD 14)

include_directories(Core converttest)



