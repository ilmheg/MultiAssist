// Based on havok2fbx by Highflex

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>    // copy
#include <iterator>     // back_inserter
#include <regex>        // regex, sregex_token_iterator

// HAVOK stuff now
#include <Common/Base/hkBase.h>
#include <Common/Base/Memory/System/Util/hkMemoryInitUtil.h>
#include <Common/Base/Memory/Allocator/Malloc/hkMallocAllocator.h>
#include <Common/Base/System/Io/IStream/hkIStream.h>

#include <Common/Base/Reflection/Registry/hkDefaultClassNameRegistry.h>
#include <Common/Serialize/Util/hkStaticClassNameRegistry.h>

#include <cstdio>

// Compatibility
#include <Common/Compat/hkCompat.h>

// Scene
#include <Common/SceneData/Scene/hkxScene.h>

#include <Common/Base/Fwd/hkcstdio.h>

// Geometry
#include <Common/Base/Types/Geometry/hkGeometry.h>

// Serialize
#include <Common/Serialize/Util/hkRootLevelContainer.h>
#include <Common/Serialize/Util/hkLoader.h>
#include <Common/Serialize/Util/hkSerializeUtil.h>
#include <Common/Serialize/Version/hkVersionPatchManager.h>
#include <Common/Serialize/Data/hkDataObject.h>

// Animation
#include <Animation/Animation/Rig/hkaSkeleton.h>
#include <Animation/Animation/hkaAnimationContainer.h>
#include <Animation/Animation/Mapper/hkaSkeletonMapper.h>
#include <Animation/Animation/Playback/Control/Default/hkaDefaultAnimationControl.h>
#include <Animation/Animation/Playback/hkaAnimatedSkeleton.h>
#include <Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h>
#include <Animation/Animation/Rig/hkaPose.h>
#include <Animation/Animation/Rig/hkaSkeletonUtils.h>

// Reflection
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkClassMember.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkClassMemberAccessor.h>

// Utils
#include "MathHelper.h"
#include "EulerAngles.h"

// FBX
#include <fbxsdk.h>
#include "FBXCommon.h" // samples common path, todo better way

// FBX Function prototypes.
bool CreateScene(FbxManager* pSdkManager, FbxScene* pScene); // create FBX scene
FbxNode* CreateSkeleton(FbxScene* pScene, const char* pName); // create the actual skeleton
void AnimateSkeleton(FbxScene* pScene); // add animation to it
hkRootLevelContainer* ConvertHavok(FbxScene* pScene);

int GetNodeIDByName(FbxScene* pScene, std::string NodeName);
FbxNode* GetNodeIndexByName(FbxScene* pScene, const std::string& NodeName);

hkVector4 GetV4(FbxVector4 vec4);
hkQuaternion GetQuat(FbxQuaternion fQuat);
hkQuaternion GetQuat(FbxVector4 fQuat);
hkQuaternion GetQuat2(FbxQuaternion fQuat);
hkQuaternion GetQuat2(FbxVector4 fQuat);
hkQsTransform* ConvertTransform(FbxAMatrix* matrix);

void PlatformInit();
void PlatformFileSystemInit();

static void HK_CALL errorReport(const char* msg, void* userContext)
{
	using namespace std;
	printf("%s", msg);
}

bool file_exists(const char* fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

// http://stackoverflow.com/questions/6417817/easy-way-to-remove-extension-from-a-filename
std::string remove_extension(const std::string& filename)
{
	size_t lastdot = filename.find_last_of('.');
	if (lastdot == std::string::npos) return filename;
	return filename.substr(0, lastdot);
}

static void show_usage()
{
	// TODO: better versioning
	std::cerr << "fbx2havok for FFXIV by perchbird\n\n"
		<< "Options:\n"
		<< "\t-h,--help\n\tShow this help message\n\n"
		<< "\t-hk_skeleton,--havokskeleton PATH\n\tSpecify the original Havok skeleton file\n\n"
		<< "\t-hk_anim,--havokanim PATH\n\tSpecify the original Havok animation file\n\n"
		<< "\t-fbx,--fbxfile PATH\n\tSpecify the FBX input file to convert\n\n"
		<< "\t-hkout,--havokout PATH\n\tSpecify the Havok output file to save\n\n"
		<< std::endl;
}

// global so we can access this later
class hkLoader* loader;
class hkaSkeleton* skeleton;
class hkaAnimation** animations;
class hkaAnimationBinding** bindings;

int numBindings;
int numAnims;

bool bAnimationGiven = false;

#define HK_GET_DEMOS_ASSET_FILENAME(fname) fname

// From hkxcmd/hkfutils.cpp
//hkResult LoadDefaultRegistry()
//{
//    hkVersionPatchManager patchManager;
//    {
//        extern void HK_CALL CustomRegisterPatches(hkVersionPatchManager& patchManager);
//        CustomRegisterPatches(patchManager);
//    }
//    hkDefaultClassNameRegistry &defaultRegistry = hkDefaultClassNameRegistry::getInstance();
//    {
//        extern void HK_CALL CustomRegisterDefaultClasses();
//        extern void HK_CALL ValidateClassSignatures();
//        CustomRegisterDefaultClasses();
//        ValidateClassSignatures();
//    }
//    return HK_SUCCESS;
//}

int HK_CALL main(int argc, const char** argv)
{
	// user needs to specify only the input file
	// if no output argument was given just assume same path as input and write file there
	if (argc < 2)
	{
		show_usage();
		return 1;
	}

	hkStringBuf havokskeleton;
	hkStringBuf havokanim;
	const char* havokout = nullptr;
	const char* fbxfile = nullptr;
	std::string havok_path_backup;

	bool bSkeletonIsValid = false;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];

		if ((arg == "-h") || (arg == "--help")) {
			show_usage();
			return 0;
		}
		else {
			// skeleton is required
			if ((arg == "-hk_skeleton") || (arg == "--havokskeleton") || (arg == "-skl")) {
				if (i + 1 < argc) {
					// check if file is valid before going on
					if (file_exists(argv[i + 1])) {
						bSkeletonIsValid = true;
						havokskeleton = argv[i + 1];
						havok_path_backup = argv[i + 1];
						std::cout << "HAVOK SKELETON FILEPATH IS: " << havokskeleton << "\n";
					}
					else {
						std::cerr << "ERROR: specified havok skeleton file doesn't exist!" << std::endl;
						return 1;
					}
				}
				else {
					std::cerr << "--havokskeleton option requires path argument." << std::endl;
					return 1;
				}
			}

			if (((arg == "-hk_anim") || (arg == "--havokanim") || (arg == "-anim")) && bSkeletonIsValid) {
				if (i + 1 < argc) {
					// check if file is valid before going on
					if (file_exists(argv[i + 1])) {
						havokanim = argv[i + 1];
						std::cout << "HAVOK ANIMATION FILEPATH IS: " << havokanim << "\n";
						bAnimationGiven = true;
					}
					else {
						std::cerr << "ERROR: specified havok animation file doesn't exist!" << std::endl;
						return 1;
					}
				}
			}

			if ((arg == "-fbx") || (arg == "--fbxfile")) {
				if (i + 1 < argc) {
					if (file_exists(argv[i + 1])) {
						fbxfile = argv[i + 1];
						std::cout << "FBX FILEPATH IS: " << fbxfile << std::endl;
					}
					else {
						std::cerr << "ERROR: Must specify FBX file to read." << std::endl;
					}
				}
				else {
					std::cerr << "--fbxfile option requires path argument." << std::endl;
					return 1;
				}
			}

			if (((arg == "-hkout") || (arg == "--havokout") || (arg == "-o")) && bSkeletonIsValid) {
                std::cout << bSkeletonIsValid;
				if (i + 1 < argc) {
					havokout = argv[i + 1];
					std::cout << "HAVOK ANIMATION OUTPUT IS: " << havokout << "\n";
				}
			}
		}
	}

	PlatformInit();

	hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault(hkMallocAllocator::m_defaultMallocAllocator, hkMemorySystem::FrameInfo(1024 * 1024));
	hkBaseSystem::init(memoryRouter, errorReport);

	PlatformFileSystemInit();
	//    LoadDefaultRegistry();

	{
		loader = new hkLoader();
		{
			hkStringBuf assetFile(havokskeleton); assetFile;
			hkRootLevelContainer* container = loader->load(HK_GET_DEMOS_ASSET_FILENAME(assetFile.cString()));
			//            container->
			HK_ASSERT2(0x27343437, container != HK_NULL, "Could not load asset");
			auto* ac = reinterpret_cast<hkaAnimationContainer*>(container->findObjectByType(hkaAnimationContainerClass.getName()));

			HK_ASSERT2(0x27343435, ac && (ac->m_skeletons.getSize() > 0), "No skeleton loaded");
			skeleton = ac->m_skeletons[0];
		}

		{
            if(bAnimationGiven){
			    hkStringBuf assetFile(havokanim);assetFile;

			    hkRootLevelContainer* container = loader->load(HK_GET_DEMOS_ASSET_FILENAME(assetFile.cString()));
			    HK_ASSERT2(0x27343437, container != HK_NULL, "Could not load asset");
			    auto* ac = reinterpret_cast<hkaAnimationContainer*>(container->findObjectByType(hkaAnimationContainerClass.getName()));

                HK_ASSERT2(0x27343435, ac && (ac->m_bindings.getSize() > 0), "No binding loaded");
                numBindings = ac->m_bindings.getSize();
                bindings = new hkaAnimationBinding * [numBindings];

                for (int i = 0; i < numBindings; i++)
                    bindings[i] = ac->m_bindings[i];
            }		

			
		}
	}

	FbxManager* lSdkManager = nullptr;
	FbxScene* lScene = nullptr;

	InitializeSdkObjects(lSdkManager, lScene);
	bool lResult = LoadScene(lSdkManager, lScene, fbxfile);

	if (!lResult)
	{
		FBXSDK_printf("\n\nAn error occurred while loading the scene...\n");
		DestroySdkObjects(lSdkManager, lResult);
		return 0;
	}

	const char* outfilename = nullptr;
	std::string fbx_extension = ".hkx";
	std::string combined_path;

	if (fbxfile != nullptr)
	{
		outfilename = havokout;
	}
	else
	{
		// get havok skel path and trim the extension from it
		combined_path = remove_extension(havok_path_backup) + fbx_extension;
		outfilename = combined_path.c_str();

		std::cout << "\n" << "Saving HKX to: " << outfilename << "\n";
	}

	auto root = ConvertHavok(lScene);
	hkVariant vRoot = { &root, &hkRootLevelContainer::staticClass() };
	hkOstream stream(outfilename);
	int flags = hkSerializeUtil::SAVE_DEFAULT;
	flags |= hkSerializeUtil::SAVE_CONCISE;

	hkPackfileWriter::Options packOptions; packOptions.m_layout = hkStructureLayout::MsvcAmd64LayoutRules;
	//    hkTagfileWriter::Options tagOptions; tagOptions.m_layout = hkStructureLayout::MsvcAmd64LayoutRules;

	//    hkResult res = hkSerializeUtil::savePackfile(root, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), packOptions, HK_NULL, (hkSerializeUtil::SaveOptionBits) flags);
	hkResult res = hkSerializeUtil::saveTagfile(root, hkRootLevelContainer::staticClass(), stream.getStreamWriter(), HK_NULL, (hkSerializeUtil::SaveOptionBits)flags);

	if (res.m_enum == hkResultEnum::HK_SUCCESS) {
        
		DestroySdkObjects(lSdkManager, lResult);
		hkBaseSystem::quit();
		hkMemoryInitUtil::quit();
		printf("Done!\n");

		return 0;
	}
	else {
		FBXSDK_printf("\n\nAn error occurred while saving the scene...\n");
		DestroySdkObjects(lSdkManager, lResult);
		return 0;
	}
}

// This wouldn't be possible without figment's hkxcmd KF conversion code.
// Thanks figment!
void CreateAnimFromStack(FbxScene* pScene, FbxAnimStack* stack, int stackNum, hkaAnimationContainer* animCont) {

	hkRefPtr<hkaAnimationBinding> newBinding = new hkaAnimationBinding();

	animCont->m_bindings.append(&newBinding, 1);

	//    int numTracks = pScene->GetSrcObjectCount<FbxNode>();
	//    int numTracks = skeleton->m_bones.getSize();
	//int numTracks = bindings[stackNum]->m_transformTrackToBoneIndices.getSize();
    std::cout << "\n\nBONES: " << skeleton->m_bones.getSize() - 1;
	int numTracks = skeleton->m_bones.getSize()-1;
	float duration = (float)stack->GetReferenceTimeSpan().GetDuration().GetSecondDouble();
	float frametime = (1.0 / 30);   //always 30fps
	int numFrames = (int)(duration / frametime);
    frametime = (float)duration / (numFrames - 1);
    std::cout << "\n\nFRAMES: " << numFrames << "\n\n";
	hkRefPtr<hkaInterleavedUncompressedAnimation> anim = new hkaInterleavedUncompressedAnimation();
	anim->m_duration = duration;
	anim->m_numberOfTransformTracks = numTracks;
	anim->m_numberOfFloatTracks = 0;
	anim->m_transforms.setSize(numTracks * numFrames, hkQsTransform::getIdentity());
	anim->m_floats.setSize(0);
	//    anim->m_annotationTracks.setSize(numTracks);

	hkArray<hkQsTransform>& transforms = anim->m_transforms;
	hkArray<hkInt16> newTransformTrack;
	newTransformTrack.setSize(numTracks);

	auto BoneIDContainer = new FbxNode * [skeleton->m_bones.getSize()];
	for (int y = 0; y < skeleton->m_bones.getSize(); y++) {
		const char* CurrentBoneName = skeleton->m_bones[y].m_name;
		std::string CurBoneNameString = CurrentBoneName;
		BoneIDContainer[y] = GetNodeIndexByName(pScene, CurrentBoneName);
		newTransformTrack[y] = y;
	}

	auto evaluator = pScene->GetAnimationEvaluator();

	FbxTime currentFbxTime(0);
	float time = 0;
	for (int frame = 0; frame < numFrames; frame++, time += frametime) {
        
		if (frame % 10 == 0)
			cout << "==\n";
		// per-frame init stuff
		currentFbxTime.SetSecondDouble(time);

		for (int track = 0; track < numTracks; track++) {
			FbxNode* node = BoneIDContainer[track+1];
			FbxAMatrix local = evaluator->GetNodeLocalTransform(node, currentFbxTime);

			hkQsTransform* hkLocal = ConvertTransform(&local);
			const hkVector4& t = hkLocal->getTranslation();
			const hkQuaternion& r = hkLocal->getRotation();
			const hkVector4& s = hkLocal->getScale();
			anim->m_transforms[frame * numTracks + track].set(t, r, s);
		}
	}
	cout << "\nFrames done.\n";

	hkaSkeletonUtils::normalizeRotations(anim->m_transforms.begin(), anim->m_transforms.getSize());

	{
		//        auto tParams = new hkaSplineCompressedAnimation::TrackCompressionParams();
		//        auto aParams = new hkaSplineCompressedAnimation::AnimationCompressionParams();
		auto outAnim = anim;
		//new hkaSplineCompressedAnimation(*anim);
		newBinding->m_animation = outAnim;

		// copy transform track to bone indices
        for (int t = 0; t < numTracks; t++) {
            newBinding->m_transformTrackToBoneIndices.pushBack(newTransformTrack[t + 1]);
        }
        cout << "Binding Skeleton";
		newBinding->m_originalSkeletonName = bindings[0]->m_originalSkeletonName;
        cout << "Skeleton Bound";
        
	}

    cout << "Packing animation container";
	animCont->m_animations.pushBack(newBinding->m_animation);
}

hkRootLevelContainer* ConvertHavok(FbxScene* pScene) {

	int numStacks = pScene->GetSrcObjectCount<FbxAnimStack>();

	auto* rootContainer = new hkRootLevelContainer();
    auto* animContainer = new hkaAnimationContainer();

    std::cout << "NS: \n" << numStacks << "\n";

    hkRefPtr<hkaAnimationContainer> animCont = new hkaAnimationContainer();

    for (int i = 0; i < numStacks; i++)
        CreateAnimFromStack(pScene, pScene->GetSrcObject<FbxAnimStack>(i), i, animContainer);

    cout << "Packing root container";
    rootContainer->m_namedVariants.pushBack(hkRootLevelContainer::NamedVariant("Merged Animation Container", animContainer, &hkaAnimationContainer::staticClass()));

   
    cout << "Root container packed";
	return rootContainer;
}

// Utility to make sure we always return the right index for the given node
// Very handy for skeleton hierachy work in the FBX SDK
FbxNode* GetNodeIndexByName(FbxScene* pScene, const std::string& NodeName)
{
	// temp hacky
	FbxNode* NodeToReturn = FbxNode::Create(pScene, "empty");

	for (int i = 0; i < pScene->GetNodeCount(); i++)
	{
		std::string CurrentNodeName = pScene->GetNode(i)->GetName();

		if (CurrentNodeName == NodeName)
			NodeToReturn = pScene->GetNode(i);
	}

	return NodeToReturn;
}

int GetNodeIDByName(FbxScene* pScene, std::string NodeName) {
	int NodeNumber = 0;

	for (int i = 0; i < pScene->GetNodeCount(); i++) {
		std::string CurrentNodeName = pScene->GetNode(i)->GetName();

		if (CurrentNodeName == NodeName) {
			NodeNumber = i;
		}
	}

	return NodeNumber;
}

hkVector4 GetV4(FbxVector4 vec4) {
	hkVector4 ret;
	ret.set(vec4.mData[0], vec4.mData[1], vec4.mData[2], vec4.mData[3]);
	return ret;
}

hkQuaternion GetQuat(FbxQuaternion fQuat) {
	hkQuaternion ret;
	ret.m_vec.set(fQuat.mData[0], fQuat.mData[1], fQuat.mData[2], fQuat.mData[3]);
	return ret;
}

hkQuaternion GetQuat(FbxVector4 fQuat) {
	hkQuaternion ret;
	ret.m_vec.set(fQuat.mData[0], fQuat.mData[1], fQuat.mData[2], fQuat.mData[3]);
	return ret;
}

hkQuaternion GetQuat2(FbxQuaternion fQuat) {
	hkVector4 v(fQuat.mData[0], fQuat.mData[1], fQuat.mData[2], fQuat.mData[3]);
	v.normalize4();
	hkQuaternion ret(v.getSimdAt(0), v.getSimdAt(1), v.getSimdAt(2), v.getSimdAt(3));
	return ret;
}

hkQuaternion GetQuat2(FbxVector4 fQuat) {
	hkVector4 v(fQuat.mData[0], fQuat.mData[1], fQuat.mData[2], fQuat.mData[3]);
	v.normalize4();
	hkQuaternion ret(v.getSimdAt(0), v.getSimdAt(1), v.getSimdAt(2), v.getSimdAt(3));
	return ret;
}

hkQsTransform* ConvertTransform(FbxAMatrix* matrix) {

	auto ret = new hkQsTransform(hkQsTransform::IdentityInitializer::IDENTITY);

	hkVector4 t = GetV4(matrix->GetT());
	hkQuaternion r = GetQuat(matrix->GetQ());
	hkVector4 s = GetV4(matrix->GetS());

	ret->set(t, r, s);
	return ret;
}

// [id=keycode]
#include <Common/Base/keycode.cxx>

// [id=productfeatures]
// We're not using anything product specific yet. We undef these so we don't get the usual
// product initialization for the products.
#undef HK_FEATURE_PRODUCT_AI
//#undef HK_FEATURE_PRODUCT_ANIMATION
#undef HK_FEATURE_PRODUCT_CLOTH
#undef HK_FEATURE_PRODUCT_DESTRUCTION_2012
#undef HK_FEATURE_PRODUCT_DESTRUCTION
#undef HK_FEATURE_PRODUCT_BEHAVIOR
#undef HK_FEATURE_PRODUCT_PHYSICS_2012
#undef HK_FEATURE_PRODUCT_SIMULATION
#undef HK_FEATURE_PRODUCT_PHYSICS

// We can also restrict the compatibility to files created with the current version only using HK_SERIALIZE_MIN_COMPATIBLE_VERSION.
// If we wanted to have compatibility with at most version 650b1 we could have used something like:
// #define HK_SERIALIZE_MIN_COMPATIBLE_VERSION 650b1.
#define HK_SERIALIZE_MIN_COMPATIBLE_VERSION 201130r1 // 201130r1 can read FFXIV Havok files
//#define HK_SERIALIZE_MIN_COMPATIBLE_VERSION 201130r1 //FFXIV is compatible with 201130r1

#include <Common/Base/Config/hkProductFeatures.cxx>

// Platform specific initialization
#include <Common/Base/System/Init/PlatformInit.cxx>
