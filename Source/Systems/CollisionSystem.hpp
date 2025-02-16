#pragma once

#include "Events/TransformChangeEvent.hpp"

#include <Freya/Freya.hpp>
#include <Freyr/Freyr.hpp>

#include <Systems/OctreeSystem.hpp>

class CollisionSystem final : public fr::System
{
  public:
    CollisionSystem(const std::shared_ptr<fr::Scene>&     scene,
                    const std::shared_ptr<fra::Renderer>& renderer,
                    const std::shared_ptr<fra::MeshPool>& meshPool,
                    const std::shared_ptr<OctreeSystem>&  octreeSystem) :
        System(scene), mRenderer(renderer), mMeshPool(meshPool),
        mOctreeSystem(octreeSystem), mChangedEntities(1000)
    {
        mScene->AddEventListener<TransformChangeEvent>(
            [this](const TransformChangeEvent& e) {
                mChangedEntities.insert(e.entity);
            });
    }

    ~CollisionSystem() override = default;

    void Update(float deltaTime) override;

  private:
    friend class SpaceApp;
    fr::SparseSet<fr::Entity> mChangedEntities;

    std::vector<std::uint32_t> mSphereModel;
    std::vector<std::uint32_t> mCubeModel;

    std::shared_ptr<fra::Renderer> mRenderer;
    std::shared_ptr<fra::MeshPool> mMeshPool;
    std::shared_ptr<OctreeSystem>  mOctreeSystem;
};