#pragma once

#include "IRenderPass.h"
#include "ParticleCommands.h"

class VertexBuffer;

class ParticlesPass : public IRenderPass
{
public:
    explicit ParticlesPass(ComPtr<ID3D12Device4> device);
    ~ParticlesPass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void renderImages(ID3D12GraphicsCommandList4* commandList);
    Matrix buildImageWorldMatrix(const ParticleCommand& command) const;
    Matrix buildImageVP();

private:
    struct Vertex
    {
        Vector2 position;
        Vector2 texCoord;
    };

private:
    const D3D12_VIEWPORT* m_viewport = nullptr;
    const std::vector<ParticleEmitterCommand>* m_commands;
    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;
    const Vector3* m_cameraPosition = nullptr;

    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::unique_ptr<VertexBuffer> m_quadVertexBuffer;

    // TEMPORARY
    
    std::vector<ParticleEmitterCommand> test = { {nullptr, 0, Vector2 (1.f, 1.f),
           {
               {
                   Vector3(0.f, 0.f, 0.f),
                   Vector2(1.f, 1.f),
                   3.14159272F,
                   Vector4(1.f, 1.f, 1.f, 1.f),
                   Vector2(0.f, 0.f)
               },
               {
                   Vector3(0.f, 5.f, 0.f),
                   Vector2(2.f, 2.f),
                   0.f,
                   Vector4(0.f, 1.f, 0.f, 1.f),
                   Vector2(0.f, 0.f)
               }
           }
       }
    };



/*
std::vector<ParticleEmitterCommand> test = { {nullptr, 0, Vector2 (1.f, 1.f),
          {
              {
                  Vector3(5.f, 1.f, 1.f),
                  Vector2(1.f, 1.f),
                  0.f,
                  Vector4(1.f, 1.f, 1.f, 1.f),
                  Vector2(0.f, 0.f)
              }
          }
      }
};
*/

// pi = 3.14159272F (just for 180 degrees rotation)
//Commands per particle:
//Vector3 position;
//Vector2 scale;
//float rotationZ;
//Vector4 colorAndAlpha;


};
