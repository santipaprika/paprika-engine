#include <Passes/Pass.h>

using namespace PPK;

Pass::Pass()
{
	m_frameDirty[0] = true;
	m_frameDirty[1] = true;
}
