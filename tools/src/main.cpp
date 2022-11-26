#pragma once

#include <iostream>
#include <confParser.hpp>
#include <fstream>
#include <sstream>

auto readFile(std::string_view path) -> std::optional<std::string> {
	std::ifstream file(path.data(), std::ios::binary);

	if (file.fail())
		return std::nullopt;

	const auto start = file.tellg();
	file.seekg(0, std::ios::end);
	const auto end = file.tellg();
	file.seekg(start);
	auto fileSize = end - start;

	std::string result;
	// Pointless memset
	result.resize(fileSize);

	file.read(result.data(), fileSize);
	if (file.fail())
		return std::nullopt;

	return result;
}

auto outputConfFileCode(const Conf::ConfigFile& conf, std::ostream& hppOut, std::ostream& cppOut) -> void {
	using namespace Conf;

	bool imGuiUsed = false;
	for (const auto& structure : conf.structs) {
		for (const auto& property : structure.properties) {
			switch (property) {
			case StructPropertyType::IM_GUI: imGuiUsed = true; break;
			default:
				break;
			}
		}
	}

	hppOut << "#include <utils/int.hpp>\n";


	if (imGuiUsed) {
		cppOut << "#include <imgui.h>\n";
	}

	cppOut << '\n';

	if (imGuiUsed) {
		cppOut << "using namespace ImGui;\n";
	}

	cppOut << '\n';

	for (const auto& structure : conf.structs) {
		hppOut << "struct " << structure.name << " {\n";

		for (const auto& field : structure.fields) {
			hppOut << '\t';
			switch (field.type.type) {
			case FieldTypeType::I32: hppOut << "i32"; break;
			case FieldTypeType::FLOAT: hppOut << "float"; break;
			}
			hppOut << " " << field.name << ";\n";
		}

		for (const auto& property : structure.properties) {
			switch (property) {
			case StructPropertyType::IM_GUI:
				hppOut << "\tauto displayGui() const -> void;\n";

				cppOut << "auto " << structure.name << "::displayGui() const -> void {\n";
				for (const auto& field : structure.fields) {
					cppOut << '\t';
					switch (field.type.type) {
					case FieldTypeType::I32: cppOut << "InputInt(\"" << field.name << "\", &" << field.name << ");\n"; break;
					case FieldTypeType::FLOAT: cppOut << "InputFloat(\"" << field.name << "\", &" << field.name << ");\n"; break;
					}
				}
				cppOut << "}\n";
				break;
			default:
				break;
			}
		}

		hppOut << "}\n\n";
	}
}

#include <filesystem>

namespace fs = std::filesystem;

auto main() -> int {
	std::cout << fs::current_path() << '\n';
	const auto path = "src/test.conf";
	const auto text = readFile("src/test.conf");
	if (!text.has_value()) {
		std::cerr << "failed to read " << path << '\n';
		return EXIT_FAILURE;
	}
	Conf::Parser parser;
	const auto conf = parser.parse(*text);
	std::stringstream hpp, cpp;
	outputConfFileCode(*conf, cpp, hpp);
	std::cout << cpp.str() << '\n' << hpp.str() << '\n';

	//const auto srcPath = fs::current_path() / "../src";
	//std::cout << srcPath << '\n';

	//for (const auto& dirEntry : fs::recursive_directory_iterator(srcPath))
	//	std::cout << dirEntry.path() << '\n';;
}