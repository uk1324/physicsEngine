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

	static constexpr auto DEBUG_PRINT_OLD_SAVED_STATE_BEFORE_ADDING_COMMAND = true;

	{
		hppOut << "#pragma once\n\n";
		hppOut << "#include <game/editor/editorGuiState.hpp>\n";
		hppOut << "struct Commands;\n";
		hppOut << "struct EditorEntities;\n";
		hppOut << "struct Entity;\n";
		hppOut << "#include <utils/typeInfo.hpp>\n";
		if (jsonUsed) hppOut << "#include <json/JsonValue.hpp>\n";
		hppOut << '\n';
	}

	{
		cppOut << "#include <" << includePath << ">\n";
		cppOut << "#include <game/editor/commands.hpp>\n";
		if (imGuiUsed) cppOut << "#include <imgui/imgui.h>\n";
		if (DEBUG_PRINT_OLD_SAVED_STATE_BEFORE_ADDING_COMMAND) cppOut << "#include <utils/io.hpp>\n";

		cppOut << '\n';

		if (imGuiUsed) cppOut << "using namespace ImGui;\n";
		if (jsonUsed) cppOut << "using namespace Json;\n";

		cppOut << '\n';
	}

	for (const auto& cppCode : conf.cppCode) {
		hppOut << cppCode << '\n';
	}

	for (const auto& structure : conf.structs) {
		hppOut << "struct " << structure.name << "Editor {\n";

		for (const auto& field : structure.fields) {
			hppOut << '\t';
			switch (field.type.type) {
			case FieldTypeType::I32: hppOut << "i32"; break;
			case FieldTypeType::FLOAT: hppOut << "float"; break;
			case FieldTypeType::VEC2: hppOut << "Vec2"; break;
			case FieldTypeType::CPP: hppOut << field.type.cpp; break;
			}
			hppOut << " " << field.name << ";\n";
		}

		hppOut << '\n';

		auto findFieldProperty = [](const Field& field, FieldPropertyType type) -> std::optional<FieldProperty> {
			for (const auto& property : field.properties) {
				if (property.type == type) {
					return property;
				}
			}
			return std::nullopt;
		};

		auto outUpperSnakeCase = [](std::ostream& os, std::string_view str) -> void {
			for (usize i = 0; i < str.size(); i++) {
				const auto c = str[i];
				if (isupper(c) && i != 0 && i != str.size() - 1)
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
			case StructPropertyType::IM_GUI: {
				
				const auto editorGuiSignature = "editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void";
				hppOut << "\tauto " << editorGuiSignature << ";\n";

				cppOut << "auto " << structure.name << "Editor::" << editorGuiSignature << " {\n";
				for (const auto& field : structure.fields) {
					const auto customProperty = findFieldProperty(field, FieldPropertyType::CUSTOM);
					if (field.type.type == FieldTypeType::CPP && !customProperty.has_value())
						continue;

					cppOut << '\t';
					switch (field.type.type) {
					case FieldTypeType::I32: cppOut << "InputInt(\"" << field.name << "\", &" << field.name << ")"; break;
					case FieldTypeType::FLOAT: cppOut << "InputFloat(\"" << field.name << "\", &" << field.name << ")"; break;
					case FieldTypeType::VEC2: cppOut << "InputFloat2(\"" << field.name << "\", " << field.name << ".data())"; break;
					case FieldTypeType::CPP: cppOut << customProperty->customGuiFn << "(" << field.name << ")"; break;
					}
					cppOut << ";\n";

					if (field.type.type != FieldTypeType::CPP) {
						cppOut << "\tif (IsItemActivated()) {\n "
							"\t\tinputState.inputing = true;\n"
							"\t\t*reinterpret_cast<decltype(" << field.name << ")*>(inputState.placeToSaveDataAfterNewChange()) = " << field.name << ";\n"
							"\t}\n";

						cppOut << "\tif (IsItemDeactivatedAfterEdit()) {\n";
						if (DEBUG_PRINT_OLD_SAVED_STATE_BEFORE_ADDING_COMMAND) {
							cppOut << "\t\tdbg(*reinterpret_cast<decltype(" << field.name << ")*>(inputState.oldSavedData()));\n";
						}
						cppOut << "\t\tcommands.addSetFieldCommand(entity, ";
						outOffsetName(cppOut, structure.name, field.name);
						cppOut << ", inputState.oldSavedData(), entites.getFieldPointer(entity, ";
						outOffsetName(cppOut, structure.name, field.name);
						cppOut << "), static_cast<u8>(sizeof(" << field.name << ")));\n";
						cppOut << "\t}\n";

						cppOut << "\tif (IsItemDeactivated()) { inputState.inputing = false; }\n\n";
					}
				}
				cppOut << "}\n\n";
				break;
			}
				

			case StructPropertyType::SERIALIZABLE:
				{
					hppOut << "\tauto toJson() const -> Json::Value;\n";

					cppOut << "auto " << structure.name << "Editor::toJson() const -> Json::Value {\n";
					cppOut << "\tauto result = Json::Value::emptyObject();\n";
					for (const auto& field : structure.fields) {
						const auto customProperty = findFieldProperty(field, FieldPropertyType::CUSTOM);

						if (field.type.type == FieldTypeType::CPP && !customProperty.has_value())
							continue;

						cppOut << "\tresult[\"" << field.name << "\"] = ";
						if (customProperty.has_value()) {
							cppOut << customProperty->customSerializeFn << "(" << field.name << ")";
						} else {
							switch (field.type.type) {
							case FieldTypeType::I32: cppOut << "Json::Value(" << field.name << ")"; break;
							case FieldTypeType::FLOAT: cppOut << "Json::Value(" << field.name << ")"; break;
							case FieldTypeType::VEC2: cppOut << "{ { \"x\", " << field.name << ".x }, { \"y\", " << field.name << ".y } }"; break;
							}
						}
						cppOut << ";\n";
					}
					cppOut << "\treturn result;\n";
					cppOut << "}\n\n";
				}

				{
					hppOut << "\tstatic auto fromJson(const Json::Value& json) -> " << structure.name << "Editor;\n";

					cppOut << "auto " << structure.name << "Editor::fromJson(const Json::Value& json) -> " << structure.name << "Editor {\n";
					cppOut << "\treturn " << structure.name << "Editor{\n";
					for (const auto& field : structure.fields) {
						const auto customProperty = findFieldProperty(field, FieldPropertyType::CUSTOM);

						if (field.type.type == FieldTypeType::CPP && !customProperty.has_value())
							continue;

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
							case FieldTypeType::FLOAT: cppOut << jsonGet(cppOut, field.name) << ".number()"; break;
							case FieldTypeType::VEC2: cppOut << "Vec2{ " << jsonGet(cppOut, field.name) << ".at(\"x\").number(), " << jsonGet(cppOut, field.name) << ".at(\"y\").number() }"; break;
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
		hppOut << "};\n\n";

		// If offsetof is inside the class it is referencing it results in an error.
		for (const auto& field : structure.fields) {
			hppOut << "static constexpr auto ";
			outOffsetName(hppOut, structure.name, field.name);
			hppOut << " = offsetof(" << structure.name << "Editor, " << field.name << ");\n";
		}
		hppOut << '\n';

		// TODO: Could use this for finding common properites on objects.
		//hppOut << "static constexpr StructField ";
		//outUpperSnakeCase(hppOut, structure.name);
		//hppOut << "_EDITOR_OFFSETS[] { ";
		//for (const auto& field : structure.fields) {
		//	hppOut << "{ ";
		//	hppOut << "\"" << field.name << "\", ";
		//	outOffsetName(hppOut, structure.name, field.name);
		//	hppOut << " }, ";
		//}
		//hppOut << "};\n\n";

		hppOut << "struct " << structure.name << " : public " << structure.name << "Editor {\n";
		for (const auto& code : structure.cppCode) {
			hppOut << '\t' << code << '\n';
		}
		hppOut << "};\n\n";
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