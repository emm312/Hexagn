#include <compiler/linker.h>

#include <iostream>
#include <string>
#include <vector>

#include <compiler/token.h>
#include <util.h>

std::vector<Function> Linker::functions;

void Linker::addFunction(const Function& function)
{
	// Check for duplicate function
	for (const auto& func: functions)
	{
		if (func.getSignature() == function.getSignature())
		{
			std::cerr << "Error: Duplicate function '" << function.name.m_val << "'\n";
			std::cerr << "Previous definition:\n";
			std::cerr << func.returnType.m_lineno << ": " << getSourceLine(glob_src, func.returnType.m_lineno);
			if (func.name.m_lineno != func.returnType.m_lineno)
			{
				std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
				drawArrows(func.name.m_start, func.name.m_end);
			}
			else
				drawArrows(func.returnType.m_start, func.name.m_end);
			exit(-1);
		}

		if (func.returnType.toString() != function.returnType.toString() && func.argTypes == function.argTypes)
		{
			std::cerr << "Cannot have functions with same arguments but different return types: " << function.name.m_val << '\n';
			std::cerr << "Previous definition:\n";
			std::cerr << func.returnType.m_lineno << ": " << getSourceLine(glob_src, func.returnType.m_lineno);
			if (func.name.m_lineno != func.returnType.m_lineno)
			{
				std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
				drawArrows(func.name.m_start, func.name.m_end);
			}
			else
				drawArrows(func.returnType.m_start, func.name.m_end);
			exit(-1);
		}
	}

	functions.push_back(function);
}

const Function Linker::getFunction(const Token& name, const std::vector<Token>& argTypes)
{
	for (const auto& func: functions)
		if (func.name.m_val == name.m_val && func.argTypes == argTypes)
			return func;

	std::cerr << "Error: Function '" << name.m_val << "' does not exist\n";
	std::cerr << name.m_lineno << ": " << getSourceLine(glob_src, name.m_lineno);
	drawArrows(name.m_start, name.m_end);
	exit(-1);
}
