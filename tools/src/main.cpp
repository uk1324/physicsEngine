#pragma once

#include <iostream>
#include <confParser.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

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

static auto operator<<(std::ostream& os, const Data::FieldType& type) -> std::ostream& {
	using namespace Data;
	switch (type.type) {
	case FieldTypeType::I32: os << "i32"; break;
	case FieldTypeType::FLOAT:
	case FieldTypeType::ANGLE:
		os << "float";
		break;
	case FieldTypeType::VEC2: os << "Vec2"; break;
	case FieldTypeType::VEC3: os << "Vec3"; break;
	case FieldTypeType::CPP: os << type.cpp; break;
	case FieldTypeType::USIZE: os << "usize"; break;
	case FieldTypeType::VARIANT:
		os << "std::variant<";
		for (usize i = 0; i < type.variant.size(); i++) {
			os << type.variant[i];
			if (i != type.variant.size() - 1) {
				os << ", ";
			}
		}
		os << ">";
		break;
	case FieldTypeType::VECTOR:
		os << "std::vector<" << type.vectorType << ">";
		break;
	}
	return os;
}

using namespace Data;

auto findFieldProperty(const Field& field, FieldPropertyType type) -> std::optional<FieldProperty> {
	for (const auto& property : field.properties) {
		if (property.type == type) {
			return property;
		}
	}
	return std::nullopt;
};

auto outFieldSerialize(std::ostream& os, const Field& field, const FieldType& type, std::string_view name) -> void {
	switch (type.type) {
	case FieldTypeType::I32: os << "Json::Value(" << field.name << ")"; break;
	case FieldTypeType::FLOAT:
	case FieldTypeType::ANGLE:
		os << "Json::Value(" << field.name << ")";
		break;
	case FieldTypeType::VEC2: os << "{ { \"x\", " << field.name << ".x }, { \"y\", " << field.name << ".y } }"; break;
	case FieldTypeType::VEC3: os << "{ { \"x\", " << field.name << ".x }, { \"y\", " << field.name << ".y }, { \"z\", " << field.name << ".z } }"; break;
	case FieldTypeType::USIZE: os << "Json::Value(static_cast<Json::Value::IntType>(" << field.name << "))"; break;
	case FieldTypeType::CPP: os << name << ".toJson()"; break;
	case FieldTypeType::VARIANT:
		break;
	}
}

auto outputConfFileCode(const Data::DataFile& conf, std::string_view includePath, std::ostream& hppOut, std::ostream& cppOut) -> void {
	using namespace Data;

	bool imGuiUsed = false;
	bool jsonUsed = false;
	for (const auto& structure : conf.structs) {
		for (const auto& property : structure.properties) {
			switch (property) {
			case StructPropertyType::IM_GUI: imGuiUsed = true; break;
			case StructPropertyType::SERIALIZABLE: jsonUsed = true; break;
			default:
				break;
			}
		}
	}

	{
		hppOut << "#pragma once\n\n";
		hppOut << "#include <math/vec2.hpp>\n";
		hppOut << "#include <math/vec3.hpp>\n";
		if (jsonUsed) hppOut << "#include <json/JsonValue.hpp>\n";
		hppOut << '\n';
	}

	{
		cppOut << "#include <" << includePath << ">\n";

		if (jsonUsed) cppOut << "using namespace Json;\n";

		cppOut << '\n';
	}

	for (const auto& code : conf.orderedCode) {
		switch (code.type) {
		case DataFile::CodeType::CPP_CODE: {
			const auto& cppCode = conf.cppCode[code.index];
			hppOut << cppCode << '\n';
		}
		break;

		case DataFile::CodeType::STRUCT: {
			const auto& structure = conf.structs[code.index];
			hppOut << "struct " << structure.name << " {\n";

			for (const auto& field : structure.fields) {
				hppOut << '\t' << field.type << " " << field.name << ";\n";
			}

			hppOut << '\n';

			auto outFirstLetterUppercase = [](std::ostream& os, std::string_view str) -> void {
				if (str.length() > 0) {
					os << static_cast<char>(toupper(str[0]));
					os << str.substr(1);
				}
			};

			auto outUpperSnakeCase = [](std::ostream& os, std::string_view str) -> void {
				for (usize i = 0; i < str.size(); i++) {
					const auto c = str[i];
					if (isupper(c) && i != 0)
						os << '_';
					os << static_cast<char>(toupper(c));
				}
			};

			auto outOffsetName = [outUpperSnakeCase](std::ostream& os, std::string_view structName, std::string_view fieldName) -> void {
				outUpperSnakeCase(os, structName);
				os << "_EDITOR_";
				outUpperSnakeCase(os, fieldName);
				os << "_OFFSET";
			};

			for (const auto& property : structure.properties) {
				switch (property) {
				case StructPropertyType::SERIALIZABLE:
				{
					hppOut << "\tauto toJson() const -> Json::Value;\n";

					cppOut << "auto " << structure.name << "::toJson() const -> Json::Value {\n";
					cppOut << "\tauto result = Json::Value::emptyObject();\n";
					for (const auto& field : structure.fields) {
						const auto customProperty = findFieldProperty(field, FieldPropertyType::CUSTOM);

						cppOut << "\tresult[\"" << field.name << "\"] = ";
						if (customProperty.has_value()) {
							cppOut << customProperty->customSerializeFn << "(" << field.name << ")";
						} else {
							outFieldSerialize(cppOut, field, field.type, field.name);
						}
						cppOut << ";\n";
					}
					cppOut << "\treturn result;\n";
					cppOut << "}\n\n";
				}

				{
					hppOut << "\tstatic auto fromJson(const Json::Value& json) -> " << structure.name << ";\n";

					cppOut << "auto " << structure.name << "::fromJson(const Json::Value& json) -> " << structure.name << " {\n";
					cppOut << "\treturn " << structure.name << "{\n";
					for (const auto& field : structure.fields) {
						const auto customProperty = findFieldProperty(field, FieldPropertyType::CUSTOM);

						cppOut << "\t\t." << field.name << " = ";

						auto jsonGet = [](std::ostream& os, std::string_view fieldName) -> char {
							os << "json.at(\"" << fieldName << "\"";
							return ')';
						};

						if (customProperty.has_value()) {
							cppOut << customProperty->customDeserializeFn << "(" << jsonGet(cppOut, field.name) << ")";
						} else {
							switch (field.type.type) {
							case FieldTypeType::I32: cppOut << jsonGet(cppOut, field.name) << ".intNumber()"; break;
							case FieldTypeType::FLOAT:
							case FieldTypeType::ANGLE:
								cppOut << jsonGet(cppOut, field.name) << ".number()"; break;

							case FieldTypeType::VEC2: cppOut << "Vec2{ " << jsonGet(cppOut, field.name) << ".at(\"x\").number(), " << jsonGet(cppOut, field.name) << ".at(\"y\").number() }"; break;
							case FieldTypeType::VEC3: cppOut << "Vec3{ " << jsonGet(cppOut, field.name) << ".at(\"x\").number(), " << jsonGet(cppOut, field.name) << ".at(\"y\").number(), " << jsonGet(cppOut, field.name) << ".at(\"z\").number() }"; break;

							case FieldTypeType::USIZE: cppOut << "static_cast<usize>(" << jsonGet(cppOut, field.name) << ".intNumber())"; break;
							case FieldTypeType::CPP: cppOut << field.type << "::fromJson(" << jsonGet(cppOut, field.name) << ")"; break;
							case FieldTypeType::VECTOR: 
								cppOut << "[&]{ " << field.type << "  }()";
								break;
							}
						}
						cppOut << ",\n";
					}
					cppOut << "\t};\n}\n\n";
				}

				break;

				default:
					break;
				}
			}
			for (const auto& code : structure.cppCode) {
				hppOut << '\t' << code << '\n';
			}

			hppOut << "};\n\n";
		}
		}
	}
}

namespace fs = std::filesystem;

// Currently if a field that doesn't exist gets access an exception is thrown. Could allow to default initialize the value if it isn't defined.

auto main() -> int {
	const auto srcPath = fs::current_path() / "../src";
	std::cout << "sourcePath: " << srcPath << '\n';
	
	Data::Parser parser;
	for (const auto& dirEntry : fs::recursive_directory_iterator(srcPath)) {
		const auto& path = dirEntry.path();
		if (dirEntry.is_regular_file() && path.extension() == ".data") {
			auto text = readFile(path.string());
			if (!text.has_value()) {
				std::cerr << "failed to read " << path << '\n';
				return EXIT_FAILURE;
			}
			// "\r\n" is treated as a double new line.
			text->erase(std::remove(text->begin(), text->end(), '\r'), text->end());

			std::cout << "parsing " << std::filesystem::relative(path, srcPath) << '\n';
			const auto conf = parser.parse(*text);
			if (!conf.has_value())
				continue;

			std::cout << "generating code\n";
			const auto dir = path.parent_path();
			const auto name = path.stem();
			auto hppPath = (dir / (name.string() + "Data.hpp"));
			auto cppPath = (dir / (name.string() + "Data.cpp"));
			std::ofstream hpp(hppPath), cpp(cppPath);
			const auto includePath = std::filesystem::relative(hppPath, srcPath);
			outputConfFileCode(*conf, includePath.string(), hpp, cpp);
		}
	}
}