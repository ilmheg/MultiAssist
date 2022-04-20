
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
    int anim_index;
    std::string outw;
    hkStringBuf out;
    hkRootLevelContainer* skl_root_container;
    hkRootLevelContainer* anim_root_container;

    // 1 = skl -> xml packfile
    // 2 = xml packfile of skl and anim -> binary tagfile
    // 3 = skl + anim -> out hk*
    // 4 = skl + anim -> xml packfile
    // 5 = xml packfile of anim -> binary tagfile
    // 6 = skl + anim - > binary tagfile
    int mode = _wtoi(nargv[1]);

    if (mode == 1 || mode == 2) {
        skl_hkt = convert_from_wstring(nargv[2]).c_str();
        out = convert_from_wstring(nargv[3]).c_str();
    }
    if (mode == 3 || mode == 6) {
        skl_hkt = convert_from_wstring(nargv[2]).c_str();
        anim_hkt = convert_from_wstring(nargv[3]).c_str();
        anim_index = _wtoi(nargv[4]);
        out = convert_from_wstring(nargv[5]).c_str();
    }
    if (mode == 4) {
        skl_hkt = convert_from_wstring(nargv[2]).c_str();
        anim_hkt = convert_from_wstring(nargv[3]).c_str();
        out = convert_from_wstring(nargv[4]).c_str();
    }
    if (mode == 5) {
        anim_hkt = convert_from_wstring(nargv[2]).c_str();
        out = convert_from_wstring(nargv[3]).c_str();
    }

    printf("Mode is %d\n", mode);
    init();
    auto loader = new hkLoader();

    if (mode == 1 || mode == 2 || mode == 3 || mode == 6) {
        skl_root_container = loader->load(skl_hkt);
    }
    if (mode == 3 || mode == 4 || mode == 6) {
        anim_root_container = loader->load(anim_hkt);
    }
    if (mode == 5) {
        anim_root_container = hkSerializeUtil::loadObject<hkRootLevelContainer>(anim_hkt);
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
    }

    if (res.isSuccess()) {
        // I had some cleanup here. And then Havok decided to access violate every time.
        return 0;
    } else {
        std::cout << "\n\nAn error occurred within animassist...\n";
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