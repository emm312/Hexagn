#include <compiler/string.h>

#include <vector>
#include <string>

struct CompilerString
{
	const std::string signature;
	const std::string value;
};

static std::vector<CompilerString> strings;
static size_t strCount = 0;

const std::string registerString(const std::string& str)
{
	for (const auto& s: strings)
		if (s.value == str)
			return s.signature;
	
	const std::string signature = ".str" + std::to_string(strCount);

	std::string newStr;
	for (const char& c: str)
		switch (c)
		{
			case '\n':
			{
				newStr += "\\n";
				break;
			}
			
			case '\t':
			{
				newStr += "\\t";
				break;
			}
			
			default:
			{
				newStr += c;
				break;
			}
		}

	strings.push_back( { signature, newStr } );
	strCount++;
	return signature;
}

const std::vector<std::string> getStrings()
{
	std::vector<std::string> retStrings;
	for (const auto& s: strings)
		retStrings.push_back(s.signature + '\n' + "DW [ \"" + s.value + "\" 0 ]");
	return retStrings;
}
