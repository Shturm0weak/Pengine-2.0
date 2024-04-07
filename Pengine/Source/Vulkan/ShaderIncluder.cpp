#include "ShaderIncluder.h"

#include "../Utils/Utils.h"

#include <array>

using namespace Pengine;
using namespace Vk;

shaderc_include_result* ShaderIncluder::GetInclude(
	const char* requestedSource,
	shaderc_include_type type,
	const char* requestingSource,
	size_t includeDepth)
{
	std::filesystem::path filepath = requestedSource;
	std::string content = Utils::ReadFile(filepath);

	std::array<std::string, 2>* userData = new std::array<std::string, 2>();
	(*userData)[0] = filepath.string();
	(*userData)[1] = content;

	shaderc_include_result* data = new shaderc_include_result();
	data->user_data = userData;
	data->source_name = userData->at(0).c_str();
	data->source_name_length = userData->at(0).size();
	data->content = userData->at(1).c_str();
	data->content_length = userData->at(1).size();

	return data;
}

void ShaderIncluder::ReleaseInclude(shaderc_include_result* data)
{
	delete static_cast<std::array<std::string, 2>*>(data->user_data);
	delete data;
}
