#include <compiler/linker.h>

#include <iostream>
#include <string>
#include <vector>

#include <compiler/token.h>
#include <util.h>

static std::vector<Function> linkerFunctions;

void linkerAddFunction(const Function& function)
{
	// Check for duplicate function
	for (const auto& func: linkerFunctions)
	{
		if (func.getSignature() == function.getSignature())
		{
			std::cerr << "Error: Duplicate function '" << function.name.m_val << "'\n";
			std::cerr << "Previous definition:\n";
			std::cerr << func.returnType.m_lineno << ": " << getSourceLine(glob_src, func.returnType.m_lineno);
			if (func.name.m_lineno != func.returnType.m_lineno)
			{
				std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
				drawArrows(func.name.m_start, func.name.m_end, func.name.m_lineno);
			}
			else
				drawArrows(func.returnType.m_start, func.name.m_end, func.returnType.m_lineno);
			exit(-1);
		}

		if (func.name.m_val == function.name.m_val && func.returnType.toString() != function.returnType.toString() && func.argTypes == function.argTypes)
		{
			std::cerr << "Cannot have functions with same arguments but different return types: " << function.name.m_val << '\n';
			std::cerr << "Previous definition:\n";
			std::cerr << func.returnType.m_lineno << ": " << getSourceLine(glob_src, func.returnType.m_lineno);
			if (func.name.m_lineno != func.returnType.m_lineno)
			{
				std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
				drawArrows(func.name.m_start, func.name.m_end, func.name.m_lineno);
			}
			else
				drawArrows(func.returnType.m_start, func.name.m_end, func.name.m_lineno);
			exit(-1);
		}
	}

	linkerFunctions.push_back(function);
}

const Function linkerGetFunction(const Token& name, const std::vector<Token>& argTypes)
{
	for (const auto& func: linkerFunctions)
		if (func.name.m_val == name.m_val && func.argTypes == argTypes)
			return func;

	std::cerr << "Error: Function '" << name.m_val << "' does not exist\n";
	std::cerr << name.m_lineno << ": " << getSourceLine(glob_src, name.m_lineno);
	drawArrows(name.m_start, name.m_end, name.m_lineno);
	exit(-1);
}

const std::vector<Function>& linkerGetFunctions()
{
	return linkerFunctions;
}
