#include <compiler/linker.h>

#include <iostream>

#include <compiler/ast/ast.h>
#include <compiler/token.h>
#include <util.h>

struct LenghtEncodedType
{
	const size_t len;
	const std::string val;
};

const LenghtEncodedType encodeType(TypeNode* typeName)
{
	const std::string type = typeName->val;
	const size_t len = type.length();
	std::string ret;
	bool isIdentifier = false;

		 if (type == "void")   ret = 'v';
	else if (type == "int8")   ret = "i8";
	else if (type == "int16")  ret = "i16";
	else if (type == "int32")  ret = "i32";
	else if (type == "int64")  ret = "i64";
	else if (type == "uint8")  ret = "u8";
	else if (type == "uint16") ret = "u16";
	else if (type == "uint32") ret = "u32";
	else if (type == "uint64") ret = "u64";
	else
	{
		ret = type;
		isIdentifier = true;
	}

	if (typeName->isPointer)
		ret += 'P';

	if (!isIdentifier)
		return { size_t(-1), ret };
	else
		return { len, ret };
}

std::string Function::getSignature() const
{
	std::stringstream ss;

	ss << "_Hx" /* Hexagn mangled name signature */ << name->name.length() << name->name;

	const LenghtEncodedType& _return = encodeType(returnType);
	if (_return.len == size_t(-1))
		ss << _return.val;
	else
		ss << '_' << _return.len << _return.val;

	for (const auto& arg: argTypes)
	{
		const LenghtEncodedType& var = encodeType(arg);
		if (var.len == size_t(-1))
			ss << var.val;
		else
			ss << '_' << var.len << var.val;
	}

	return ss.str();
}

void Linker::addFunction(const Function& function)
{
	// Check for duplicate function
	for (const auto& func: linkerFunctions)
	{
		TypeNode* retType = func.returnType;

		if (func.getSignature() == function.getSignature())
		{
			std::cerr << "Error: Duplicate function '" << function.name->name << "'\n";
			// std::cerr << "Previous definition:\n";
			// std::cerr << retType.m_lineno << ": " << getSourceLine(glob_src, retType.m_lineno);
			// if (func.name.m_lineno != retType.m_lineno)
			// {
			// 	std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
			// 	drawArrows(func.name.m_start, func.name.m_end, func.name.m_lineno);
			// }
			// else
			// 	drawArrows(retType.m_start, func.name.m_end, retType.m_lineno);
			exit(-1);
		}

		if (func.name->name == function.name->name && retType->val != function.returnType->val && func.argTypes == function.argTypes)
		{
			std::cerr << "Cannot have functions with same arguments but different return types: " << function.name->name << '\n';
			// std::cerr << "Previous definition:\n";
			// std::cerr << retType.m_lineno << ": " << getSourceLine(glob_src, retType.m_lineno);
			// if (func.name.m_lineno != retType.m_lineno)
			// {
			// 	std::cerr << func.name.m_lineno << ": " << getSourceLine(glob_src, func.name.m_lineno);
			// 	drawArrows(func.name.m_start, func.name.m_end, func.name.m_lineno);
			// }
			// else
			// 	drawArrows(retType.m_start, func.name.m_end, func.name.m_lineno);
			exit(-1);
		}
	}

	linkerFunctions.push_back(function);
}

Function Linker::getFunction(const std::string& src, const std::string& name, const std::vector<TypeNode*>& argTypes) const
{
	for (const auto& func: linkerFunctions)
	{
		if (func.name->name == name)
		{
			if (func.argTypes.size() != argTypes.size()) break;
			for (size_t i = 0; i < func.argTypes.size(); ++i)
			{
				if (isIntegerDataType(func.argTypes[i]))
				{
					if (!isIntegerDataType(argTypes[i]) && !isNumber(argTypes[i]))
						goto nextFunc;
				}
				else if (isFloatDataType(func.argTypes[i]) && !isFloatDataType(argTypes[i]))
					goto nextFunc;
				else if (func.argTypes[i]->val == "string" && argTypes[i]->val != "string")
					goto nextFunc;
			}

			return func;
		}

		nextFunc:;
	}

	std::cerr << "Error: Function '" << name << "' with arguments ";
	for (const auto& arg: argTypes)
		std::cerr << arg->val << ' ';
	std::cerr << "does not exist\n";
	exit(-1);
}

const std::vector<Function>& Linker::getFunctions() const
{
	return linkerFunctions;
}
