
#include <iostream>
#include <cstdio>

#include <Common/Base/hkBase.h>
#include <Common/Base/Memory/System/Util/hkMemoryInitUtil.h>
#include <Common/Base/System/Io/IStream/hkIStream.h>
#include <Common/Base/Reflection/Registry/hkDefaultClassNameRegistry.h>
#include <Common/Serialize/Util/hkRootLevelContainer.h>
#include <Common/Serialize/Util/hkLoader.h>
#include <Common/Serialize/Util/hkSerializeUtil.h>
#include <Common/Serialize/Version/hkVersionPatchManager.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Serialize/Util/hkSerializeDeprecated.h>

#include <Animation/Animation/hkaAnimationContainer.h>
#include <Animation/Animation/Animation/Util/hkaAdditiveAnimationUtility.h>
#include <Animation/Animation/Animation/PredictiveCompressed/hkaPredictiveCompressedAnimation.h>
#include <shellapi.h>
#include <locale>
#include <codecvt>

#include "Common/Base/System/Init/PlatformInit.cxx"

static void HK_CALL errorReport(const char* msg, void* userContext)
{
    using namespace std;
    printf("%s", msg);
}

void init() {
    PlatformInit();
    hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault(hkMallocAllocator::m_defaultMallocAllocator, hkMemorySystem::FrameInfo(1024 * 1024));
    hkBaseSystem::init(memoryRouter, errorReport);
    PlatformFileSystemInit();
    hkSerializeDeprecatedInit::initDeprecated();
}

inline std::string convert_from_wstring(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conv;
    return conv.to_bytes(wstr);
}

// animassist.exe (1) in_skl_hkx out_skl_xml
// animassist.exe (2) in_edited_hk_xml out_anim_hkx
// animassist.exe (3) in_skl_hkx in_anim_hkx anim_index out_merged_hkx
// animassist.exe (4) in_anim_hkx out_anim_xml
// animassist.exe (5) in_anim_hkx out_anim_xml
// animassist.exe (6) in_anim_packfile in_skl_hkx out_anim_hkx
// animassist.exe (7) in_anim_hkx out_additive_hkx
// animassist.exe (8) in_mod_hkx in_anim_hkx  out_repacked_hkx
// animassist.exe (9) in_anim_hkx out_precitive_hkx
int main(int argc, const char** argv) {

    int nargc = 0;
    wchar_t** nargv;

    auto command_line = GetCommandLineW();
    if (command_line == nullptr)
    {
        printf("Fatal error.");
        return 1;
    }
    nargv = CommandLineToArgvW(command_line, &nargc);
    if (nargv == nullptr)
    {
        printf("Fatal error.");
        return 1;
    }

    hkStringBuf skl_hkt;
    hkStringBuf anim_hkt;
	hkStringBuf mod_hkt;
    int anim_index;
    std::string outw;
    hkStringBuf out;
    hkRootLevelContainer* skl_root_container;
    hkRootLevelContainer* anim_root_container;
	hkRootLevelContainer* mod_root_container;

    // 1 = skl -> xml packfile
    // 2 = xml packfile of skl and anim -> binary tagfile
    // 3 = skl + anim -> out hk*
    // 4 = skl + anim -> xml packfile
    // 5 = xml packfile of anim -> binary tagfile
    // 6 = skl + anim - > binary tagfile
    // 7 = anim - > additive tagfile
    // 8 = mod_anim + anim - > binary tagfile
    // 9 = anim - > predictive compressed animation tagfile
    int mode = _wtoi(nargv[1]);

    if (mode == 1 || mode == 2) {
        skl_hkt = convert_from_wstring(nargv[2]).c_str();
        out = convert_from_wstring(nargv[3]).c_str();
    }
    if (mode == 3 || mode == 6 || mode == 8) {
        skl_hkt = convert_from_wstring(nargv[2]).c_str();
        anim_hkt = convert_from_wstring(nargv[3]).c_str();
        anim_index = _wtoi(nargv[4]);
        out = convert_from_wstring(nargv[5]).c_str();
    }
    if (mode == 4 || mode == 7 || mode == 9) {
        skl_hkt = convert_from_wstring(nargv[2]).c_str();
        anim_hkt = convert_from_wstring(nargv[3]).c_str();
        out = convert_from_wstring(nargv[4]).c_str();
    }
    if (mode == 5 ) {
        anim_hkt = convert_from_wstring(nargv[2]).c_str();
        out = convert_from_wstring(nargv[3]).c_str();
    }


    printf("Mode is %d\n", mode);
    init();
    auto loader = new hkLoader();

    if (mode == 1 || mode == 2 || mode == 3 || mode == 6 || mode == 7 || mode == 9) {
        skl_root_container = loader->load(skl_hkt);
    }
    if (mode == 3 || mode == 4 || mode == 6 ) {
        anim_root_container = loader->load(anim_hkt);
    }
    if (mode == 5 || mode == 7 || mode == 8 || mode == 9) {
        anim_root_container = hkSerializeUtil::loadObject<hkRootLevelContainer>(anim_hkt);
    }
	if (mode == 8) {
        mod_root_container = hkSerializeUtil::loadObject<hkRootLevelContainer>(skl_hkt);
    }

    hkOstream stream(out);
    hkPackfileWriter::Options packOptions;
    hkSerializeUtil::ErrorDetails errOut;

    auto layoutRules = hkStructureLayout::HostLayoutRules;
    layoutRules.m_bytesInPointer = 8;
    if (mode != 4) {
        packOptions.m_layout = layoutRules;
    }

    hkResult res;
    if (mode == 1) {
        auto* skl_container = reinterpret_cast<hkaAnimationContainer*>(skl_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        res = hkSerializeDeprecated::getInstance().saveXmlPackfile(skl_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), packOptions, nullptr, &errOut);
    } else if (mode == 2) {
        auto* skl_container = reinterpret_cast<hkaAnimationContainer*>(skl_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        skl_container->m_skeletons.clear();
        res = hkSerializeUtil::saveTagfile(skl_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), nullptr, hkSerializeUtil::SAVE_DEFAULT);
    } else if (mode == 3 || mode == 6) {
        auto* skl_container = reinterpret_cast<hkaAnimationContainer*>(skl_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        auto anim_container = reinterpret_cast<hkaAnimationContainer*>(anim_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        auto anim_ptr = anim_container->m_animations[anim_index];
        auto binding_ptr = anim_container->m_bindings[anim_index];
        auto anim_ref = hkRefPtr<hkaAnimation>(anim_ptr);
        auto binding_ref = hkRefPtr<hkaAnimationBinding>(binding_ptr);
        skl_container->m_animations.append(&anim_ref, 1);
        skl_container->m_bindings.append(&binding_ref, 1);
		if (mode == 3) {
			res = hkSerializeUtil::savePackfile(skl_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), packOptions, nullptr, hkSerializeUtil::SAVE_DEFAULT);
		} else if (mode == 6) {
			res = hkSerializeUtil::saveTagfile(skl_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), nullptr, hkSerializeUtil::SAVE_DEFAULT);
		}
    } else if (mode == 4) {
        packOptions.m_writeMetaInfo = true;
        packOptions.m_writeSerializedFalse = false;
        res = hkSerializeDeprecated::getInstance().saveXmlPackfile(anim_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), packOptions, nullptr, &errOut);
    } else if (mode == 5) {
        res = hkSerializeUtil::saveTagfile(anim_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), nullptr, hkSerializeUtil::SAVE_DEFAULT);
    } else if (mode == 7) {
        auto* skl_container = reinterpret_cast<hkaAnimationContainer*>(skl_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        hkaAnimationContainer* anim_container = reinterpret_cast<hkaAnimationContainer*>(anim_root_container->findObjectByType(hkaAnimationContainerClass.getName()));;
        hkaAnimation* anim_ptr = anim_container->m_animations[0];
        auto bind_ptr = anim_container->m_bindings[0];
        auto binding_ref = hkRefPtr<hkaAnimationBinding>(bind_ptr);
        auto skl_ptr = skl_container->m_skeletons[0];
        hkaInterleavedUncompressedAnimation* w = static_cast<hkaInterleavedUncompressedAnimation*>(anim_ptr);

        hkaAdditiveAnimationUtility::ReferencePoseInput rinput;
        rinput.m_originalData = w->m_transforms.begin();
        rinput.m_numberOfPoses = w->m_transforms.getSize() / w->m_numberOfTransformTracks;
        rinput.m_numberOfTransformTracks = w->m_numberOfTransformTracks;
        rinput.m_referencePose = skl_ptr->m_referencePose.begin();
        rinput.m_numReferencePose = skl_ptr->m_referencePose.getSize();
        rinput.m_transformTrackToBoneIndices = bind_ptr->m_transformTrackToBoneIndices.begin();
        rinput.m_numTransformTrackToBoneIndices = bind_ptr->m_transformTrackToBoneIndices.getSize();
        /*
        hkaAdditiveAnimationUtility::Input input;
        input.m_numberOfPoses = w->m_transforms.getSize() / w->m_numberOfTransformTracks; //frames
        input.m_numberOfTransformTracks = w->m_numberOfTransformTracks;
        input.m_originalData = w->m_transforms.begin();
        input.m_baseData = skl_ptr->m_referencePose.begin();
        */
        hkaAdditiveAnimationUtility::createAdditiveFromReferencePose(rinput, w->m_transforms.begin());
        
        /// Note: For deprecated format, uncomment the define HKA_USE_ADDITIVE_DEPRECATED in hkaAnimation.h
        /// also remember to set hkaAnimationBinding::ADDITIVE_DEPRECATED
        /// Couldn't get results from ADDITVE_DEPRECATED, ADDITIVE works in XIV, so here we are
        /// Source\Animation\Animation\hkaAnimation.h Line #18
        binding_ref->m_blendHint = hkaAnimationBinding::ADDITIVE;
        anim_container->m_animations[0] = w;
        anim_container->m_bindings[0] = binding_ref;

        res = hkSerializeDeprecated::getInstance().saveXmlPackfile(anim_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), packOptions, nullptr, &errOut);
    } else if (mode == 8) {
        hkaAnimationContainer* anim_container = reinterpret_cast<hkaAnimationContainer*>(anim_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        hkaAnimationContainer* mod_container = reinterpret_cast<hkaAnimationContainer*>(mod_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        auto mod_anim_ptr = mod_container->m_animations[0];
        auto mod_binding_ptr = mod_container->m_bindings[0];
        mod_binding_ptr->m_originalSkeletonName = anim_container->m_bindings[anim_index]->m_originalSkeletonName;
        mod_anim_ptr->m_annotationTracks.clear();
        anim_container->m_animations[anim_index] = mod_anim_ptr;
        anim_container->m_bindings[anim_index] = mod_binding_ptr;
        res = hkSerializeUtil::saveTagfile(anim_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), nullptr, hkSerializeUtil::SAVE_DEFAULT);
    } else if (mode == 9) {
        auto* skl_container = reinterpret_cast<hkaAnimationContainer*>(skl_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        auto skl = *skl_container->m_skeletons[0];

        auto* anim_container = reinterpret_cast<hkaAnimationContainer*>(anim_root_container->findObjectByType(hkaAnimationContainerClass.getName()));
        auto binding_ptr = anim_container->m_bindings[0];
        auto binding = *anim_container->m_bindings[0];

        /*
        hkaPredictiveCompressedAnimation::TrackCompressionParams track_params;
        hkaPredictiveCompressedAnimation::CompressionParams params;
        track_params.m_dynamicFloatTolerance = 1;
        track_params.m_dynamicScaleTolerance = 1;
        track_params.m_dynamicRotationTolerance = 0.005;
        track_params.m_dynamicTranslationTolerance = 0.005;
        track_params.m_staticFloatTolerance = 1;
        track_params.m_staticScaleTolerance = 1;
        track_params.m_staticRotationTolerance = 0.005;
        track_params.m_staticTranslationTolerance = 0.005;
        params.m_parameterPalette[0] = track_params;
        */
        auto compressed_anim = new hkaPredictiveCompressedAnimation(binding, skl);
        anim_container->m_animations[0] = compressed_anim;
        binding_ptr->m_animation = compressed_anim;

        res = hkSerializeDeprecated::getInstance().saveXmlPackfile(anim_root_container, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), packOptions, nullptr, &errOut);
    }

    if (res.isSuccess()) {
        hkBaseSystem::quit();
        hkMemoryInitUtil::quit();
        return 0;
    } else {
        std::cout << "\n\nAn error occurred within animassist...\n";
        hkBaseSystem::quit();
        hkMemoryInitUtil::quit();
        return 1;
    }
}

#include <Common/Base/keycode.cxx>

#undef HK_FEATURE_PRODUCT_AI
//#undef HK_FEATURE_PRODUCT_ANIMATION
#undef HK_FEATURE_PRODUCT_CLOTH
#undef HK_FEATURE_PRODUCT_DESTRUCTION_2012
#undef HK_FEATURE_PRODUCT_DESTRUCTION
#undef HK_FEATURE_PRODUCT_BEHAVIOR
#undef HK_FEATURE_PRODUCT_PHYSICS_2012
#undef HK_FEATURE_PRODUCT_SIMULATION
#undef HK_FEATURE_PRODUCT_PHYSICS

#define HK_SERIALIZE_MIN_COMPATIBLE_VERSION 201130r1

#include <Common/Base/Config/hkProductFeatures.cxx>