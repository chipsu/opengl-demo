#include "Model.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

inline glm::vec3 make_vec3(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
inline glm::vec2 make_vec2(const aiVector2D& v) { return glm::vec2(v.x, v.y); }
inline glm::quat make_quat(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }
inline glm::mat4 make_mat4(const aiMatrix4x4& m) { return glm::transpose(glm::make_mat4(&m.a1)); }

const aiNode* FindMeshRoot(const aiNode* node) {
    if (node->mNumMeshes) {
        return node;
    }
    for (size_t i = 0; i < node->mNumChildren; ++i) {
        auto n = FindMeshRoot(node->mChildren[i]);
        if (nullptr != n) return n;
    }
    return nullptr;
}

glm::mat4 GetNodeTransform(const aiNode* node) {
    if (node->mParent) {
        return GetNodeTransform(node->mParent) * make_mat4(node->mTransformation);
    }
    return make_mat4(node->mTransformation);
}

glm::mat4 FindGlobalInverseTransform(const aiScene* scene) {
    const auto node = FindMeshRoot(scene->mRootNode);
    auto transform = GetNodeTransform(node ? node : scene->mRootNode);
    transform = glm::inverse(transform);
    std::cout << "global inverse: " << glm::to_string(transform) << std::endl;
    std::cout << "From node: " << (node ? node->mName.data : scene->mRootNode->mName.data) << std::endl;
    return transform;
}

void LoadBoneWeights(Model* model, Mesh_ mesh, const aiMesh* nodeMesh) {
    size_t numUnmappedWeights = 0;
    for (unsigned int boneIndex = 0; boneIndex < nodeMesh->mNumBones; ++boneIndex) {
        const auto bone = nodeMesh->mBones[boneIndex];
        for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
            const auto boneWeight = bone->mWeights[weightIndex];
            assert(boneWeight.mVertexId < mesh->mVertices.size());
            const auto boneID = model->mAnimationSet->MapBone(bone->mName.data, make_mat4(bone->mOffsetMatrix));
            if (!mesh->mVertices[boneWeight.mVertexId].AddBoneWeight(boneID, boneWeight.mWeight)) {
                numUnmappedWeights++;
            }
        }
    }
    if (numUnmappedWeights > 0) {
        std::cerr << "Mesh " << nodeMesh->mName.data << ": MAX_VERTEX_WEIGHTS (" << MAX_VERTEX_WEIGHTS << ") reached for " << numUnmappedWeights << " weights" << std::endl;
    }
}

void LoadNode(Model* model, const aiScene* scene, const aiNode* node, const glm::mat4& parentTransform) {
    glm::mat4 combinedTransform = parentTransform * make_mat4(node->mTransformation);

    for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex) {
        const auto nodeMesh = scene->mMeshes[node->mMeshes[meshIndex]]; // TODO: Shared meshes?
        auto mesh = std::make_shared<Mesh>();
        auto debugColor = RandomColor();
        
        mesh->mVertices.resize(nodeMesh->mNumVertices);
        auto vertexPointer = mesh->mVertices.data();
        for (unsigned int vertexIndex = 0; vertexIndex < nodeMesh->mNumVertices; ++vertexIndex) {
            const auto& nodeVertex = nodeMesh->mVertices[vertexIndex];
            vertexPointer->mPos = make_vec3(nodeVertex);
            if (nodeMesh->HasNormals()) {
                vertexPointer->mNormal = make_vec3(nodeMesh->mNormals[vertexIndex]);
            }
            vertexPointer->mColor = debugColor; // FIXME
            vertexPointer++;
        }

        mesh->mIndices.resize(nodeMesh->mNumFaces * 3);
        auto indexPointer = mesh->mIndices.data();
        size_t invalidFaces = 0;
        for (unsigned int faceIndex = 0; faceIndex < nodeMesh->mNumFaces; ++faceIndex) {
            const auto nodeFace = &nodeMesh->mFaces[faceIndex];
            //assert(nodeFace->mNumIndices == 3);
            if (nodeFace->mNumIndices != 3) {
                invalidFaces++;
                continue;
            }
            *indexPointer++ = nodeFace->mIndices[0];
            *indexPointer++ = nodeFace->mIndices[1];
            *indexPointer++ = nodeFace->mIndices[2];
        }

        // FIXME
        if (invalidFaces > 0) {
            std::cerr << "Model has " << invalidFaces << " invalid (non triangular) faces" << std::endl;
            while (invalidFaces > 0) {
                mesh->mIndices.pop_back();
                invalidFaces--;
            }
        }

        // FIXME
        if (model->mAnimationSet) {
            LoadBoneWeights(model, mesh, nodeMesh);
        }

        model->mMeshes.push_back(std::make_shared<ModelMesh>(mesh, combinedTransform));
    }

    for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        LoadNode(model, scene, node->mChildren[childIndex], combinedTransform);
    }
}

AnimationNode_ LoadHierarchy(Model* model, const aiNode* node, AnimationNode_ parent = nullptr) {
    AnimationNode_ animationNode = std::make_shared<AnimationNode>(node->mName.data, parent, make_mat4(node->mTransformation));

    for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        auto childNode = LoadHierarchy(model, node->mChildren[childIndex], animationNode);
        animationNode->mChildren.push_back(childNode);
    }

    return animationNode;
}

void LoadAnimations(Model* model, const aiScene* scene) {
    if (scene->mNumAnimations < 1) return;

    // FIXME: Store elsewhere?
    if (!model->mAnimationSet) {
        model->mAnimationSet = std::make_shared<AnimationSet>();
        model->mAnimationSet->mGlobalInverseTransform = FindGlobalInverseTransform(scene); // FIXME
    }

    for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex) {
        const auto aAnimation = scene->mAnimations[animationIndex];
        if (!aAnimation->mNumChannels) continue; // TODO: Stuff
        auto animation = std::make_shared<Animation>();
        animation->mName = aAnimation->mName.data;
        animation->mTicksPerSecond = (float)aAnimation->mTicksPerSecond;
        animation->mDuration = (float)aAnimation->mDuration;
        animation->mRootNode = LoadHierarchy(model, scene->mRootNode);

        for (unsigned int channelIndex = 0; channelIndex < aAnimation->mNumChannels; ++channelIndex) {
            const auto aChannel = aAnimation->mChannels[channelIndex];
            auto track = std::make_shared<AnimationTrack>();
            track->mName = aChannel->mNodeName.data;

            for (unsigned int i = 0; i < aChannel->mNumPositionKeys; ++i) {
                const auto aKey = aChannel->mPositionKeys[i];
                track->mPositionKeys.push_back(VectorKey((float)aKey.mTime, make_vec3(aKey.mValue)));
            }
            for (unsigned int i = 0; i < aChannel->mNumScalingKeys; ++i) {
                const auto aKey = aChannel->mScalingKeys[i];
                track->mScalingKeys.push_back(VectorKey((float)aKey.mTime, make_vec3(aKey.mValue)));
            }
            for (unsigned int i = 0; i < aChannel->mNumRotationKeys; ++i) {
                const auto aKey = aChannel->mRotationKeys[i];
                track->mRotationKeys.push_back(QuatKey((float)aKey.mTime, make_quat(aKey.mValue)));
            }

            // TODO Pre/post state
            animation->mAnimationTracks.push_back(track);
        }

        model->mAnimationSet->mAnimations.push_back(animation);
    }
}

std::string GetMetaDataString(const aiMetadataEntry& entry) {
    switch (entry.mType) {
    case AI_BOOL: return "(BOOL) " + std::to_string(*(bool*)entry.mData);
    case AI_INT32: return "(INT32) " + std::to_string(*(int32_t*)entry.mData);
    case AI_UINT64: return "(UINT64) " + std::to_string(*(uint64_t*)entry.mData);
    case AI_FLOAT: return "(FLOAT) " + std::to_string(*(float*)entry.mData);
    case AI_DOUBLE: return "(DOUBLE) " + std::to_string(*(double*)entry.mData);
    case AI_AISTRING: {
        const auto val = (aiString*)entry.mData;
        return "(AISTRING) " + std::string(val->data);
    }
    case AI_AIVECTOR3D: {
        const auto val = make_vec3(*(aiVector3D*)entry.mData);
        return "(AIVECTOR3D) " + glm::to_string(val);
    }
    default:
        return "(UNKNOWN)";
    }
}

void FixSceneUpAxis(const aiScene* scene) {
    int32_t upAxis, upAxisSign;
    int32_t frontAxis, frontAxisSign;
    int32_t coordAxis, coordAxisSign;
    scene->mMetaData->Get("UpAxis", upAxis);
    scene->mMetaData->Get("UpAxisSign", upAxisSign);
    scene->mMetaData->Get("FrontAxis", frontAxis);
    scene->mMetaData->Get("FrontAxisSign", frontAxisSign);
    scene->mMetaData->Get("CoordAxis", coordAxis);
    scene->mMetaData->Get("CoordAxisSign", coordAxisSign);
    aiVector3D upVec = upAxis == 0 ? aiVector3D(upAxisSign, 0, 0) : upAxis == 1 ? aiVector3D(0, upAxisSign, 0) : aiVector3D(0, 0, upAxisSign);
    aiVector3D forwardVec = frontAxis == 0 ? aiVector3D(frontAxisSign, 0, 0) : frontAxis == 1 ? aiVector3D(0, frontAxisSign, 0) : aiVector3D(0, 0, frontAxisSign);
    aiVector3D rightVec = coordAxis == 0 ? aiVector3D(coordAxisSign, 0, 0) : coordAxis == 1 ? aiVector3D(0, coordAxisSign, 0) : aiVector3D(0, 0, coordAxisSign);
    aiMatrix4x4 mat(
        rightVec.x, rightVec.y, rightVec.z, 0.0f,
        upVec.x, upVec.y, upVec.z, 0.0f,
        forwardVec.x, forwardVec.y, forwardVec.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    scene->mRootNode->mTransformation = mat;
}

const aiScene* LoadScene(const std::string& fileName, const ModelOptions& options) {
    // FIXME: config
    auto props = aiCreatePropertyStore();
    //aiSetImportPropertyFloat(props, AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, options.mScale);
    auto target = /*aiProcess_GlobalScale |*/ aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights;
    auto scene = aiImportFileExWithProperties(fileName.c_str(), target, nullptr, props);
    aiReleasePropertyStore(props);
    
    if (nullptr == scene) {
        throw new std::runtime_error(aiGetErrorString());
    }
    if (scene->mMetaData) {
        for (unsigned int i = 0; i < scene->mMetaData->mNumProperties; ++i) {
            std::cout << scene->mMetaData->mKeys[i].data << "=" << GetMetaDataString(scene->mMetaData->mValues[i]) << std::endl;
        }
    }
    scene->mRootNode->mTransformation.Scaling(aiVector3D(options.mScale, options.mScale, options.mScale), scene->mRootNode->mTransformation); // FIXME
    return scene;
}

void Model::Load(const std::string& fileName, const ModelOptions& options) {
    const auto scene = LoadScene(fileName, options);
    mName = fileName;
    mAnimationSet.reset();
    LoadAnimations(this, scene);
    LoadNode(this, scene, scene->mRootNode, glm::identity<glm::mat4>());
    aiReleaseImport(scene);
    UpdateAABB();
}

void Model::LoadAnimation(const std::string& fileName, const ModelOptions& options, bool append) {
    const auto scene = LoadScene(fileName, options);
    if (!append) {
        mAnimationSet.reset();
    }
    LoadAnimations(this, scene);
}
