#include "RenderSystem.hpp"

#include "Components/ModelComponent.hpp"
#include "Components/TransformComponent.hpp"

#include "InputSystem.hpp"

void RenderSystem::Start()
{
    // mCubeModel = mMeshPool->CreateMeshFromFile("C:/Models/debug_cube.obj");
}

void RenderSystem::PostUpdate(float dt)
{
    mRenderer->BeginFrame();

    auto [view, projection] = mRenderer->GetCurrentProjection();

    const auto frustum = Frustum(projection * view);

    auto renderables = std::vector<Particle*>();
    renderables.reserve(2'000);
    mManager->StartTraceProfiling("Query renderables");
    mOctreeSystem->GetOctree()->Query(frustum, renderables);
    mManager->EndTraceProfiling();

    if (renderables.empty())
    {
        mRenderer->EndFrame();
        return;
    }

    std::ranges::sort(renderables,
        [this](const Particle* a, const Particle* b) {
            return mManager->GetComponent<ModelComponent>(a->entity).meshes <
                   mManager->GetComponent<ModelComponent>(b->entity).meshes;
        });

    auto matrices = std::vector<glm::mat4>();
    matrices.reserve(renderables.size());

    mManager->StartTraceProfiling("Calculate matrizes");
    for (const auto particle : renderables)
    {
        matrices.push_back(particle->transform.GetModel());
    }
    mManager->EndTraceProfiling();

    mInstanceMatrixBuffers =
        mRenderer->GetBufferBuilder()
            .SetData(matrices.data())
            .SetSize(sizeof(glm::mat4) * matrices.size())
            .SetUsage(fra::BufferUsage::Instance)
            .Build();

    mRenderer->BindBuffer(mInstanceMatrixBuffers);

    auto instanceDraws = std::vector<InstanceDraw>();

    mManager->StartTraceProfiling("Calculate instance sequence");
    auto                        dataIndex     = 0;
    auto                        instanceCount = 0;
    std::vector<std::uint32_t>* currentMeshes = nullptr;
    for (int i = 0; i < renderables.size(); i++)
    {
        const auto& particle = renderables[i];
        const auto& model =
            mManager->GetComponent<ModelComponent>(particle->entity);

        if (currentMeshes && currentMeshes != model.meshes)
        {
            instanceDraws.emplace_back(dataIndex, instanceCount, currentMeshes);
            currentMeshes = nullptr;
            instanceCount = 0;
            dataIndex     = i;
        }

        if (i == renderables.size() - 1)
        {
            if (!currentMeshes)
                currentMeshes = model.meshes;

            instanceDraws.emplace_back(
                dataIndex, instanceCount + 1, currentMeshes);
        }
        else
        {
            currentMeshes = model.meshes;
            instanceCount += 1;
        }
    }
    mManager->EndTraceProfiling();

    mManager->StartTraceProfiling("Draw instance sequences");
    for (const auto& draw : instanceDraws)
    {
        if (draw.meshes != nullptr)
            for (const auto& meshId : *draw.meshes)
            {
                mMeshPool->DrawInstanced(
                    meshId, draw.instanceCount, draw.index);
            }
    }
    mManager->EndTraceProfiling();

    mManager->StartTraceProfiling("Render");
    mRenderer->EndFrame();
    mManager->EndTraceProfiling();
}
