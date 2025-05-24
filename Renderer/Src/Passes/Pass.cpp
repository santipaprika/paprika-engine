#include <Passes/Pass.h>

using namespace PPK;

Pass::Pass(const wchar_t* name)
	: m_name(name)
{
	m_frameDirty[0] = true;
	m_frameDirty[1] = true;
}
