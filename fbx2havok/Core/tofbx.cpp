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
    std::cerr << "havok2fbx Version 0.1a public by Highflex 2015 (slightly modified for XIV\n\n"
        << "Options:\n"
        << "\t-h,--help\n\tShow this help message\n\n"
        << "\t-hk_skeleton,--havokskeleton PATH\n\tSpecify the Havok skeleton file\n\n"
        << "\t-hk_anim,--havokanim PATH\n\tSpecify the Havok animation file\n\n"
        << "\t-fbx,--fbxfile PATH\n\tSpecify the FBX output file to save\n\n"
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
    const char* fbxfile = nullptr;
    std::string havok_path_backup;

    bool bSkeletonIsValid = false;


    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if ((arg == "-h") || (arg == "--help"))
        {
            show_usage();
            return 0;
        }
        else
        {
            // skeleton is required
            if ((arg == "-hk_skeleton") || (arg == "--havokskeleton") || (arg == "-skl"))
            {
                if (i + 1 < argc)
                {
                    // check if file is valid before going on
                    if (file_exists(argv[i + 1]))
                    {
                        bSkeletonIsValid = true;
                        havokskeleton = argv[i + 1];
                        havok_path_backup = argv[i + 1];
                        std::cout << "HAVOK SKELETON FILEPATH IS: " << havokskeleton << "\n";
                    }
                    else
                    {
                        std::cerr << "ERROR: specified havok skeleton file doesn't exist!" << std::endl;
                        return 1;
                    }
                }
                else
                {
                    std::cerr << "--havokskeleton option requires path argument." << std::endl;
                    return 1;
                }
            }

            if (((arg == "-hk_anim") || (arg == "--havokanim") || (arg == "-anim")) && bSkeletonIsValid)
            {
                if (i + 1 < argc)
                {
                    // check if file is valid before going on
                    if (file_exists(argv[i + 1]))
                    {
                        havokanim = argv[i + 1];
                        std::cout << "HAVOK ANIMATION FILEPATH IS: " << havokanim << "\n";
                        bAnimationGiven = true;
                    }
                    else
                    {
                        std::cerr << "ERROR: specified havok animation file doesn't exist!" << std::endl;
                        return 1;
                    }
                }
            }

            if ((arg == "-fbx") || (arg == "--fbxfile"))
            {
                if (i + 1 < argc)
                {
                    fbxfile = argv[i + 1];
                    std::cout << "FBX FILEPATH IS: " << fbxfile << "\n";
                }
                else
                {
                    std::cerr << "--fbxfile option requires path argument." << std::endl;
                    return 1;
                }
            }
        }
    }

    PlatformInit();

    hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault(hkMallocAllocator::m_defaultMallocAllocator, hkMemorySystem::FrameInfo(1024 * 1024));
    hkBaseSystem::init(memoryRouter, errorReport);

    PlatformFileSystemInit();

    {
        loader = new hkLoader();
        {
			hkStringBuf assetFile(havokskeleton);
            hkRootLevelContainer* container = loader->load(HK_GET_DEMOS_ASSET_FILENAME(assetFile.cString()));
            HK_ASSERT2(0x27343437, container != HK_NULL, "Could not load asset");
            auto* ac = reinterpret_cast<hkaAnimationContainer*>(container->findObjectByType(hkaAnimationContainerClass.getName()));

            HK_ASSERT2(0x27343435, ac && (ac->m_skeletons.getSize() > 0), "No skeleton loaded");
            skeleton = ac->m_skeletons[0];
        }

        // if we do not have any animation specified proceed to exporting the skeleton data otherwise use animation
        // Get the animation and the binding
        if (bAnimationGiven)
        {
			hkStringBuf assetFile(havokanim);

            hkRootLevelContainer* container = loader->load(HK_GET_DEMOS_ASSET_FILENAME(assetFile.cString()));
            HK_ASSERT2(0x27343437, container != HK_NULL, "Could not load asset");
            auto* ac = reinterpret_cast<hkaAnimationContainer*>(container->findObjectByType(hkaAnimationContainerClass.getName()));

            HK_ASSERT2(0x27343435, ac && (ac->m_animations.getSize() > 0), "No animation loaded");
            numAnims = ac->m_animations.getSize();
            animations = new hkaAnimation * [numAnims];
            for (int i = 0; i < numAnims; i++)
                animations[i] = ac->m_animations[i];

            HK_ASSERT2(0x27343435, ac && (ac->m_bindings.getSize() > 0), "No binding loaded");
            numBindings = ac->m_bindings.getSize();
            bindings = new hkaAnimationBinding * [numBindings];

            for (int i = 0; i < numBindings; i++)
                bindings[i] = ac->m_bindings[i];
        }
        // todo delete stuff after usage
    }

    // after gathering havok data, write the stuff now into a FBX
    // INIT FBX
    FbxManager* lSdkManager = nullptr;
    FbxScene* lScene = nullptr;
    InitializeSdkObjects(lSdkManager, lScene);
    bool lResult = CreateScene(lSdkManager, lScene);

    if (!lResult)
    {
        FBXSDK_printf("\n\nAn error occurred while creating the scene...\n");
        DestroySdkObjects(lSdkManager, lResult);
        return 0;
    }

    // Save the scene to FBX.
    // check if the user has given a FBX filename/path if not use the same location as the havok input
    // The example can take an output file name as an argument.
    const char* lSampleFileName = nullptr;
    std::string fbx_extension = ".fbx";
    std::string combined_path;

    if (fbxfile != nullptr)
    {
        lSampleFileName = fbxfile;
    }
    else
    {
        // get havok skel path and trim the extension from it
        combined_path = remove_extension(havok_path_backup) + fbx_extension;
        lSampleFileName = combined_path.c_str();

        std::cout << "\n" << "Saving FBX to: " << lSampleFileName << "\n";
    }

    lResult = SaveScene(lSdkManager, lScene, lSampleFileName);

    if (lResult) {

        // Destroy all objects created by the FBX SDK.
        DestroySdkObjects(lSdkManager, lResult);

        // destroy objects not required
        hkBaseSystem::quit();
        hkMemoryInitUtil::quit();

        return 0;
    }
    else {
        FBXSDK_printf("\n\nAn error occurred while saving the scene...\n");
        DestroySdkObjects(lSdkManager, lResult);
        return 0;
    }
}

bool CreateScene(FbxManager* pSdkManager, FbxScene* pScene)
{
    // create scene info
    FbxDocumentInfo* sceneInfo = FbxDocumentInfo::Create(pSdkManager, "SceneInfo");
    sceneInfo->mTitle = "Converted Havok File";
    sceneInfo->mSubject = "A file converted from Havok formats to FBX using havok2fbx.";
    sceneInfo->mAuthor = "havok2fbx";
    sceneInfo->mRevision = "rev. 1.0";
    sceneInfo->mKeywords = "havok animation";
    sceneInfo->mComment = "no particular comments required.";
    pScene->GetGlobalSettings().SetTimeMode(FbxTime::eFrames30);
    pScene->GetGlobalSettings().SetCustomFrameRate(30.0);
    //pScene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
    //pScene->GetGlobalSettings().SetOriginalSystemUnit(FbxSystemUnit::cm);

    //FbxAxisSystem directXAxisSys(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::EFrontVector::eParityEven, FbxAxisSystem::eRightHanded);
    //directXAxisSys.ConvertScene(pScene);

    pScene->SetSceneInfo(sceneInfo);
    FbxNode* lSkeletonRoot = CreateSkeleton(pScene, "Skeleton");

    // Build the node tree.
    FbxNode* lRootNode = pScene->GetRootNode();
    lRootNode->AddChild(lSkeletonRoot);

    // Animation only if specified
    if (bAnimationGiven)
        AnimateSkeleton(pScene);

    return true;
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

int GetNodeIDByName(FbxScene* pScene, std::string NodeName);

// Create the skeleton first
FbxNode* CreateSkeleton(FbxScene* pScene, const char* pName)
{
    // get number of bones and apply reference pose
    const int numBones = skeleton->m_bones.getSize();
    
    std::cout << "\nSkeleton file has been loaded!" << " Number of Bones: " << numBones << "\n";

    // create base limb objects first
    for (hkInt16 b = 0; b < numBones; b++)
    {
        const hkaBone& bone = skeleton->m_bones[b];

        hkQsTransform localTransform = skeleton->m_referencePose[b];
        const hkVector4& pos = localTransform.getTranslation();
        const hkQuaternion& rot = localTransform.getRotation();

        FbxSkeleton* lSkeletonLimbNodeAttribute1 = FbxSkeleton::Create(pScene, pName);

        if (b == 0)
            lSkeletonLimbNodeAttribute1->SetSkeletonType(FbxSkeleton::eRoot);
        else
            lSkeletonLimbNodeAttribute1->SetSkeletonType(FbxSkeleton::eLimbNode);

        lSkeletonLimbNodeAttribute1->Size.Set(1.0);
        FbxNode* BaseJoint = FbxNode::Create(pScene, bone.m_name);
        BaseJoint->SetNodeAttribute(lSkeletonLimbNodeAttribute1);

        // Set Translation
        BaseJoint->LclTranslation.Set(FbxVector4(pos.getSimdAt(0), pos.getSimdAt(1), pos.getSimdAt(2)));

        // convert quat to euler
        Quat QuatTest = { rot.m_vec.getSimdAt(0), rot.m_vec.getSimdAt(1), rot.m_vec.getSimdAt(2), rot.m_vec.getSimdAt(3) };
        EulerAngles inAngs = Eul_FromQuat(QuatTest, EulOrdXYZs);
        BaseJoint->LclRotation.Set(FbxVector4(rad2deg(inAngs.x), rad2deg(inAngs.y), rad2deg(inAngs.z)));

        pScene->GetRootNode()->AddChild(BaseJoint);
    }

    // process parenting and transform now
    for (int c = 0; c < numBones; c++)
    {
        const hkInt32& parent = skeleton->m_parentIndices[c];

        if (parent != -1)
        {
            const char* ParentBoneName = skeleton->m_bones[parent].m_name;
            const char* CurrentBoneName = skeleton->m_bones[c].m_name;
            std::string CurBoneNameString = CurrentBoneName;
            std::string ParentBoneNameString = ParentBoneName;

            FbxNode* ParentJointNode = GetNodeIndexByName(pScene, ParentBoneName);
            FbxNode* CurrentJointNode = GetNodeIndexByName(pScene, CurrentBoneName);
            ParentJointNode->AddChild(CurrentJointNode);
        }
    }

    return pScene->GetRootNode();
}

// Create animation stack.
void AnimateSkeleton(FbxScene* pScene) {
    for (int a = 0; a < numAnims; a++) {
        FbxString lAnimStackName;
        FbxTime lTime;
        int lKeyIndex = 0;

        // First animation stack
        // TODO: add support for reading in multipile havok anims into a single FBX container
        lAnimStackName = "HavokAnimation";
        FbxAnimStack* lAnimStack = FbxAnimStack::Create(pScene, lAnimStackName);

        FbxAnimLayer* lAnimLayer = FbxAnimLayer::Create(pScene, "Base Layer");
        lAnimStack->AddMember(lAnimLayer);

        // havok related animation stuff now
        const int numBones = skeleton->m_bones.getSize();

        int FrameNumber = animations[a]->getNumOriginalFrames();
		int TrackNumber = numBones-1;
        int FloatNumber = animations[a]->m_numberOfFloatTracks;
        hkReal incrFrame = animations[a]->m_duration / (hkReal)(FrameNumber - 1);

        if (TrackNumber > numBones) {
            FBXSDK_printf("\nERROR: Number of tracks is not equal to bones\n");
            return;
        }

        //        hkLocalArray<float> floatsOut(FloatNumber);
        hkLocalArray<hkQsTransform> transformOut(TrackNumber);
        //        floatsOut.setSize(FloatNumber);
        transformOut.setSize(TrackNumber);
        hkReal startTime = 0.0;

        hkReal time = startTime;

        // used to store the bone id used inside the FBX scene file
        int* BoneIDContainer;
        BoneIDContainer = new int[numBones];

        // store IDs once to cut down process time
        // TODO utilize for skeleton code aswell
        for (int y = 0; y < numBones; y++) {
            const char* CurrentBoneName = skeleton->m_bones[y].m_name;
            std::string CurBoneNameString = CurrentBoneName;
            BoneIDContainer[y] = GetNodeIDByName(pScene, CurrentBoneName);

            //std::cout << "\n Bone:" << CurBoneNameString << " ID: " << BoneIDContainer[y] << "\n";
        }

        auto* animatedSkeleton = new hkaAnimatedSkeleton(skeleton);
        auto* control = new hkaDefaultAnimationControl(bindings[a]);
        animatedSkeleton->addAnimationControl(control);

        hkaPose pose(animatedSkeleton->getSkeleton());
        
        // loop through keyframes
        for (int iFrame = 0; iFrame < FrameNumber; ++iFrame, time += incrFrame) {
            control->setLocalTime(time);
            animatedSkeleton->sampleAndCombineAnimations(pose.accessUnsyncedPoseModelSpace().begin(), pose.getFloatSlotValues().begin());
            const hkQsTransform* transforms = pose.getSyncedPoseModelSpace().begin();
            // assume 1-to-1 transforms
            // loop through animated bones
            for (int i = 0; i < TrackNumber; ++i) {
				
                // i+1 to skip root node
                FbxNode* CurrentJointNode = pScene->GetNode(BoneIDContainer[i+1]);

                //if (std::string(skeleton->m_bones[i].m_name).find("sippo") != std::string::npos) {
                //    std::cout << "\n Bone:" << skeleton->m_bones[i].m_name << " ID: " << BoneIDContainer[i] << "\n";
                //}
                
            
                bool pCreate = iFrame == 0;


                // Translation
                FbxAnimCurve* lCurve_Trans_X = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, pCreate);
                FbxAnimCurve* lCurve_Trans_Y = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, pCreate);
                FbxAnimCurve* lCurve_Trans_Z = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, pCreate);

                // Rotation
                FbxAnimCurve* lCurve_Rot_X = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, pCreate);
                FbxAnimCurve* lCurve_Rot_Y = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, pCreate);
                FbxAnimCurve* lCurve_Rot_Z = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, pCreate);

                // Scale
                FbxAnimCurve* lCurve_Scal_X = CurrentJointNode->LclScaling.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, pCreate);
                FbxAnimCurve* lCurve_Scal_Y = CurrentJointNode->LclScaling.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, pCreate);
                FbxAnimCurve* lCurve_Scal_Z = CurrentJointNode->LclScaling.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, pCreate);

                hkQsTransform transform = transforms[i+1];
                const hkVector4 anim_pos = transform.getTranslation();
                const hkQuaternion anim_rot = transform.getRotation();
                const hkVector4f anim_scal = transform.getScale();

                // todo support for anything else beside 30 fps?
                lTime.SetTime(0, 0, 0, iFrame, 0, 0, lTime.eFrames30);

                // Translation first
                lCurve_Trans_X->KeyModifyBegin();
                lKeyIndex = lCurve_Trans_X->KeyAdd(lTime);
                lCurve_Trans_X->KeySetValue(lKeyIndex, anim_pos.getSimdAt(0));
                lCurve_Trans_X->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationConstant);
                lCurve_Trans_X->KeyModifyEnd();

                lCurve_Trans_Y->KeyModifyBegin();
                lKeyIndex = lCurve_Trans_Y->KeyAdd(lTime);
                lCurve_Trans_Y->KeySetValue(lKeyIndex, anim_pos.getSimdAt(1));
                lCurve_Trans_Y->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationConstant);
                lCurve_Trans_Y->KeyModifyEnd();

                lCurve_Trans_Z->KeyModifyBegin();
                lKeyIndex = lCurve_Trans_Z->KeyAdd(lTime);
                lCurve_Trans_Z->KeySetValue(lKeyIndex, anim_pos.getSimdAt(2));
                lCurve_Trans_Z->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationConstant);
                lCurve_Trans_Z->KeyModifyEnd();

                // Rotation
                Quat QuatRotNew = {
                        anim_rot.m_vec.getSimdAt(0),
                        anim_rot.m_vec.getSimdAt(1),
                        anim_rot.m_vec.getSimdAt(2),
                        anim_rot.m_vec.getSimdAt(3)
                };
                EulerAngles inAngs_Animation = Eul_FromQuat(QuatRotNew, EulOrdXYZs);

                lCurve_Rot_X->KeyModifyBegin();
                lKeyIndex = lCurve_Rot_X->KeyAdd(lTime);
                lCurve_Rot_X->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.x)));
                lCurve_Rot_X->KeyModifyEnd();

                lCurve_Rot_Y->KeyModifyBegin();
                lKeyIndex = lCurve_Rot_Y->KeyAdd(lTime);
                lCurve_Rot_Y->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.y)));
                lCurve_Rot_Y->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationConstant);
                lCurve_Rot_Y->KeyModifyEnd();

                lCurve_Rot_Z->KeyModifyBegin();
                lKeyIndex = lCurve_Rot_Z->KeyAdd(lTime);
                lCurve_Rot_Z->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.z)));
                lCurve_Rot_Z->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationConstant);
                lCurve_Rot_Z->KeyModifyEnd();

                // Scale
                lCurve_Scal_X->KeyModifyBegin();
                lKeyIndex = lCurve_Scal_X->KeyAdd(lTime);
                lCurve_Scal_X->KeySetValue(lKeyIndex, anim_scal.getSimdAt(0));
                lCurve_Scal_X->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationConstant);
                lCurve_Scal_X->KeyModifyEnd();

                lCurve_Scal_Y->KeyModifyBegin();
                lKeyIndex = lCurve_Scal_Y->KeyAdd(lTime);
                lCurve_Scal_Y->KeySetValue(lKeyIndex, anim_scal.getSimdAt(1));
                lCurve_Scal_Y->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationConstant);
                lCurve_Scal_Y->KeyModifyEnd();

                lCurve_Scal_Z->KeyModifyBegin();
                lKeyIndex = lCurve_Scal_Z->KeyAdd(lTime);
                lCurve_Scal_Z->KeySetValue(lKeyIndex, anim_scal.getSimdAt(2));
                lCurve_Scal_Z->KeySetInterpolation(lKeyIndex, FbxAnimCurveDef::eInterpolationConstant);
                lCurve_Scal_Z->KeyModifyEnd();
            }
        }
        

    }
}

int GetNodeIDByName(FbxScene* pScene, std::string NodeName) {
    int NodeNumber = 0;

    for (int i = 0; i < pScene->GetNodeCount(); i++)
    {
        std::string CurrentNodeName = pScene->GetNode(i)->GetName();

        if (CurrentNodeName == NodeName)
        {
            NodeNumber = i;
        }
    }

    return NodeNumber;
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
#define HK_SERIALIZE_MIN_COMPATIBLE_VERSION 201130r1 //FFXIV is compatible with 201130r1

#include <Common/Base/Config/hkProductFeatures.cxx>

// Platform specific initialization
#include <Common/Base/System/Init/PlatformInit.cxx>