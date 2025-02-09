#include "ComponentManager.h"

ComponentManager::ComponentManager()
{
    // Avoid frequent reallocations when initializing scene (filling a bunch of components)
    // TODO: Check perf difference vector vs array
    m_transformComponents.resize(32);
    m_meshComponents.resize(32);
    m_cameraComponents.resize(32);
    // TODO: Careful if we keep vectors. Reallocating could mess up buffers with copy constructor
}
