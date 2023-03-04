#pragma once

#include <fstream>
#include <sstream>
#include <filesystem>

#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>

class StreamReader : public Microsoft::glTF::IStreamReader
{
public:
   StreamReader()
       : m_streams()
   {
   }

   std::shared_ptr<std::istream> GetInputStream(const std::string& uri) const override
   {
       return GetStream(uri);
   }

private:
   std::shared_ptr<std::ifstream> GetStream(const std::string& uri) const
   {
       if (m_streams.find(uri) == m_streams.end())
       {
           m_streams[uri] = std::make_shared<std::ifstream>(std::filesystem::path(uri), std::ios_base::binary);
       }
       return m_streams[uri];
   }

   mutable std::unordered_map<std::string, std::shared_ptr<std::ifstream>> m_streams;
};

namespace PPK
{
    class GLTFReader
    {

    public:
        GLTFReader()
        {
            m_streamReader = std::make_shared<StreamReader>();
            m_gltfResourceReader = std::make_unique<Microsoft::glTF::GLTFResourceReader>(m_streamReader);
            //m_glbResourceReader = std::make_unique<Microsoft::glTF::GLBResourceReader>(this);
        }

        [[nodiscard]] Microsoft::glTF::Document GetDocument(const std::string& pathInAssets) const
        {
            std::filesystem::path filepath = std::string(ASSETS_PATH"/") + pathInAssets;
            std::filesystem::path pathFile = filepath.filename();
            std::filesystem::path pathFileExt = filepath.extension();

            std::string manifest;

            auto MakePathExt = [](const std::string& ext)
            {
                return "." + ext;
            };

            // If the file has a '.gltf' extension then create a GLTFResourceReader
            if (pathFileExt == MakePathExt(Microsoft::glTF::GLTF_EXTENSION))
            {
                auto gltfStream = m_streamReader->GetInputStream(filepath.u8string()); // Pass a UTF-8 encoded filename to GetInputString
                std::stringstream manifestStream;

                // Read the contents of the glTF file into a string using a std::stringstream
                manifestStream << gltfStream->rdbuf();
                manifest = manifestStream.str();
            }

            // If the file has a '.glb' extension then create a GLBResourceReader. This class derives
            // from GLTFResourceReader and adds support for reading manifests from a GLB container's
            // JSON chunk and resource data from the binary chunk.
            if (pathFileExt == MakePathExt(Microsoft::glTF::GLB_EXTENSION))
            {
                //auto glbStream = GetInputStream(pathFile.u8string()); // Pass a UTF-8 encoded filename to GetInputString
                //auto glbResourceReader = std::make_unique<Microsoft::glTF::GLBResourceReader>(std::move(stream), std::move(glbStream));

                //manifest = glbResourceReader->GetJson(); // Get the manifest from the JSON chunk
            }

            if (!m_gltfResourceReader && !m_glbResourceReader)
            {
                throw std::runtime_error("Command line argument path filename extension must be .gltf or .glb");
            }

            Microsoft::glTF::Document document;

            try
            {
                document = Microsoft::glTF::Deserialize(manifest);
            }
            catch (const Microsoft::glTF::GLTFException& ex)
            {
                std::stringstream ss;

                ss << "Microsoft::glTF::Deserialize failed: ";
                ss << ex.what();

                throw std::runtime_error(ss.str());
            }

            std::vector<uint32_t> indices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(document, *m_gltfResourceReader, document.meshes.Get("0").primitives[0]);
            //m_gltfResourceReader->ReadBinaryData(document, document.accessors.Get(document.meshes.GetIndex("POSITION")))
            return document;
        };

    private:
        std::shared_ptr<StreamReader> m_streamReader;
        std::unique_ptr<Microsoft::glTF::GLTFResourceReader> m_gltfResourceReader;
        std::unique_ptr<Microsoft::glTF::GLTFResourceReader> m_glbResourceReader;
    };
}
