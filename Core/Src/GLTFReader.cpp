#include <filesystem>

#include <stdafx.h>
#include <GLTFReader.h>
#include <GLTFSDK/Deserialize.h>

#include <fstream>
#include <sstream>

namespace PPK
{
	void GLTFReader::TryInitializeResourceReader()
	{
        if (m_gltfResourceReader && m_streamReader)
        {
            return;
        }

        m_streamReader = std::make_shared<StreamReader>();
        m_gltfResourceReader = std::make_unique<Microsoft::glTF::GLTFResourceReader>(m_streamReader);
        //m_glbResourceReader = std::make_unique<Microsoft::glTF::GLBResourceReader>(this);
	}

    Microsoft::glTF::Document GLTFReader::GetDocument(const std::string& pathInAssets)
    {
        // Initialize readers if it's the first read
        TryInitializeResourceReader();

        std::filesystem::path filepath = GetAssetFullFilesystemPath(pathInAssets);
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
            auto gltfStream = m_streamReader->GetInputStream(filepath.string()); // Pass a UTF-8 encoded filename to GetInputString
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

        return document;
    }
}

std::shared_ptr<std::istream> StreamReader::GetInputStream(const std::string& uri) const
{
    return GetStream(uri);
}

std::shared_ptr<std::ifstream> StreamReader::GetStream(const std::string& uri) const
{
    if (m_streams.find(uri) == m_streams.end())
    {
        m_streams[uri] = std::make_shared<std::ifstream>(std::filesystem::path(uri), std::ios_base::binary);
    }
    return m_streams[uri];
}
