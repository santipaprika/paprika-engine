#pragma once

#include <RHI/GPUResource.h>

#define MAX_TEXTURE_SUBRESOURCE_COUNT 6

namespace DirectX
{
    struct Image;
    struct TexMetadata;
}

namespace PPK
{
    namespace RHI
    {
        // Descriptor heap sampler; NOT USED ATM in favor of static samplers.
        class Sampler
        {
        public:
            Sampler(std::shared_ptr<DescriptorHeapElement> samplerHeapElement);
            Sampler(const Sampler&) = default;
            ~Sampler();

            static std::shared_ptr<Sampler> CreateSampler(LPCWSTR name = L"DefaultSampler");

        private:
            std::shared_ptr<DescriptorHeapElement> m_descriptorHeapElement;
        };
    }
}