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

glm::mat4 FindGlobalInverseTransform(const aiScene* scene) {
    const auto node = FindMeshRoot(scene->mRootNode);
    auto transform = make_mat4(node ? node->mTransformation : scene->mRootNode->mTransformation);
    transform = glm::inverse(transform);
    std::cout << "global inverse: " << glm::to_string(transform) << std::endl;
    std::cout << "From node: " << (node ? node->mName.data : scene->mRootNode->mName.data) << std::endl;
    return transform;
}

void ValidateMesh(Mesh_ mesh, AnimationController_ animationController) {
    assert(mesh->mIndices.size() > 0);
    for (size_t i = 0; i < mesh->mIndices.size(); ++i) {
        assert(mesh->mIndices[i] < mesh->mVertices.size());
    }

    assert(mesh->mVertices.size() > 0);
    for (size_t i = 0; i < mesh->mVertices.size(); ++i) {
        const Vertex& vertex = mesh->mVertices[i];

        assert(fabs(vertex.mPos.x) < 9999.0f);
        assert(fabs(vertex.mPos.y) < 9999.0f);
        assert(fabs(vertex.mPos.z) < 9999.0f);

        if (nullptr != animationController) {
            float weightTotal = 0.0f;
            assert(vertex.mBoneWeights[0] > 0.0f);
            for (int j = 0; j < MAX_VERTEX_WEIGHTS; ++j) {
                weightTotal += vertex.mBoneWeights[j];
                assert(vertex.mBoneIndices[j] < animationController->mBoneMappings.size());
            }
            //float weightDiff = fabs(1.0f - weightTotal);
            //assert(weightDiff < 0.1f);
        }
    }
}

void LoadBoneWeights(Model* model, Mesh_ mesh, const aiMesh* nodeMesh) {
    size_t numUnmappedWeights = 0;
    for (unsigned int boneIndex = 0; boneIndex < nodeMesh->mNumBones; ++boneIndex) {
        const auto bone = nodeMesh->mBones[boneIndex];
        for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
            const auto boneWeight = bone->mWeights[weightIndex];
            assert(boneWeight.mVertexId < mesh->mVertices.size());
            const auto boneID = model->mAnimationController->MapBone(bone->mName.data, make_mat4(bone->mOffsetMatrix));
            if (!mesh->mVertices[boneWeight.mVertexId].AddBoneWeight(boneID, boneWeight.mWeight)) {
                numUnmappedWeights++;
            }
        }
    }
    if (numUnmappedWeights > 0) {
        std::cerr << "Mesh " << nodeMesh->mName.data << ": MAX_VERTEX_WEIGHTS (" << MAX_VERTEX_WEIGHTS << ") reached for " << numUnmappedWeights << " weights" << std::endl;
    }
}

void LoadNode(Model* model, const aiScene* scene, const aiNode* node) {
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
        if (model->mAnimationController) {
            LoadBoneWeights(model, mesh, nodeMesh);
        }

        //ValidateMesh(mesh, model->mAnimationController);

        model->mMeshes.push_back(mesh);
    }

    for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        LoadNode(model, scene, node->mChildren[childIndex]);
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
    //if (scene->mNumAnimations < 1) return;

    // FIXME: Store elsewhere?
    if (!model->mAnimationController) {
        model->mAnimationController = std::make_shared<AnimationController>();
        model->mAnimationController->mGlobalInverseTransform = FindGlobalInverseTransform(scene); // FIXME
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

        model->mAnimationController->mAnimations.push_back(animation);
    }
}

const aiScene* LoadScene(const std::string& fileName) {
    // FIXME: config
    auto target = aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights;
    auto scene = aiImportFile(fileName.c_str(), target);
    if (nullptr == scene) {
        throw new std::runtime_error(aiGetErrorString());
    }
    return scene;
}

void Model::Load(const std::string& fileName) {
    const auto scene = LoadScene(fileName);
    mMeshes.clear();
    mAnimationController.reset();
    LoadAnimations(this, scene);
    LoadNode(this, scene, scene->mRootNode);
    aiReleaseImport(scene);
    UpdateAABB();
}

void Model::LoadAnimation(const std::string& fileName, bool append) {
    const auto scene = LoadScene(fileName);
    if (!append) {
        mAnimationController.reset();
    }
    LoadAnimations(this, scene);
}