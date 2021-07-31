#include "Util.h"

std::vector<std::string> Util::SplitString(const std::string& data, char c)
{
	std::vector<std::string> results;
	size_t pos = 0;
	while (pos < data.size())
	{
		size_t n_pos = data.find(c, pos);
		if (std::string::npos == n_pos)
		{
			auto str = data.substr(pos);
			if (!str.empty()) results.push_back(str);
			break;
		}
		auto str = data.substr(pos, n_pos - pos);
		if (!str.empty()) results.push_back(str);
		pos = n_pos + 1;
	}
	return results;
}