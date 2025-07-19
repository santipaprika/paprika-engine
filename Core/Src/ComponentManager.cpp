#include "ComponentManager.h"

ComponentManager::ComponentManager()
{
    // Reserve all memory needed beforehand to prevent runtime reallocations
    // TODO: Check perf difference vector vs array
    m_transformComponents.resize(128);
    m_meshComponents.resize(128);
    m_cameraComponents.resize(128);
    // TODO: Careful if we keep vectors. Reallocating could mess up buffers with copy constructor
}
