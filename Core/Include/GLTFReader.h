#pragma once

#include <GLTFSDK/GLTFResourceReader.h>

class StreamReader : public Microsoft::glTF::IStreamReader
{
public:
   StreamReader()
       : m_streams()
   {
   }

   std::shared_ptr<std::istream> GetInputStream(const std::string& uri) const override;

private:
    std::shared_ptr<std::ifstream> GetStream(const std::string& uri) const;

   mutable std::unordered_map<std::string, std::shared_ptr<std::ifstream>> m_streams;
};

namespace PPK
{
    class GLTFReader
    {

    public:
        static void TryInitializeResourceReader();

        [[nodiscard]] static Microsoft::glTF::Document GetDocument(const std::string& pathInAssets);

        inline static std::shared_ptr<StreamReader> m_streamReader;
        inline static std::unique_ptr<Microsoft::glTF::GLTFResourceReader> m_gltfResourceReader{};
        inline static std::unique_ptr<Microsoft::glTF::GLTFResourceReader> m_glbResourceReader{};
    };
}
