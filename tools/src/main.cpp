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

auto outFieldEditor(std::ostream& os, const Field& field, const FieldType& type, std::string_view name) -> void {
	if (type.type == FieldTypeType::USIZE)
		return;

	os << "\tTableNextRow();\n"
		"\tTableSetColumnIndex(0);\n"
		"\tAlignTextToFramePadding();\n"
		"\tText(\"" << name << "\");\n";

	os << "\tTableSetColumnIndex(1);\n"
		"\tSetNextItemWidth(-FLT_MIN);\n";

	os << "\t";
	switch (type.type) {
	case FieldTypeType::I32: os << "InputInt(\"##" << name << "\", &" << name << ")"; break;
	case FieldTypeType::FLOAT: os << "InputFloat(\"##" << name << "\", &" << name << ")"; break;
	case FieldTypeType::ANGLE: os << "inputAngle(\"##" << name << "\", &" << name << ")"; break;
	case FieldTypeType::VEC2: os << "InputFloat2(\"##" << name << "\", " << name << ".data())"; break;
	case FieldTypeType::CPP: {
		const auto customProperty = findFieldProperty(field, FieldPropertyType::CUSTOM);
		if (customProperty.has_value()) {
			os << customProperty->customGuiFn << "(" << name << ")"; 
		} else {
			os << name << ".editorGui(inputState, entites, entity, commands)";
		}
		break;
	}
	case FieldTypeType::USIZE:
		break;
	case FieldTypeType::VARIANT:
		for (const auto& type : type.variant) {
			os << "\tif (const auto value = std::get_if<" << type << ">(&" << name << ")){\n";
			os << "\t\t";
			outFieldEditor(os, field, type, "(*value)");
			os << ";\n\t}\n";
			os << "\n";
		}
		break;
	}
	os << ";\n";
	os << "\tNextColumn();\n";
}

auto outFieldSerialize(std::ostream& os, const Field& field, const FieldType& type, std::string_view name) -> void {
	switch (type.type) {
	case FieldTypeType::I32: os << "Json::Value(" << field.name << ")"; break;
	case FieldTypeType::FLOAT:
	case FieldTypeType::ANGLE:
		os << "Json::Value(" << field.name << ")";
		break;
	case FieldTypeType::VEC2: os << "{ { \"x\", " << field.name << ".x }, { \"y\", " << field.name << ".y } }"; break;
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

	static constexpr auto DEBUG_PRINT_OLD_SAVED_STATE_BEFORE_ADDING_COMMAND = true;

	{
		hppOut << "#pragma once\n\n";
		hppOut << "#include <game/editor/customGuis.hpp>\n";
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

	for (const auto& code : conf.orderedCode) {
		switch (code.type) {
		case DataFile::CodeType::CPP_CODE: {
			const auto& cppCode = conf.cppCode[code.index];
			hppOut << cppCode << '\n';
		}
		break;

		case DataFile::CodeType::STRUCT: {
			const auto& structure = conf.structs[code.index];
			hppOut << "struct " << structure.name << "Editor {\n";

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
				case StructPropertyType::IM_GUI: {

					const auto editorGuiSignature = "editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void";
					hppOut << "\tauto " << editorGuiSignature << ";\n";

					cppOut << "auto " << structure.name << "Editor::" << editorGuiSignature << " {\n";

					for (const auto& field : structure.fields) {
						
						outFieldEditor(cppOut, field, field.type, field.name);
						

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
					hppOut << "\tstatic auto fromJson(const Json::Value& json) -> " << structure.name << "Editor;\n";

					cppOut << "auto " << structure.name << "Editor::fromJson(const Json::Value& json) -> " << structure.name << "Editor {\n";
					cppOut << "\treturn " << structure.name << "Editor{\n";
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

							case FieldTypeType::USIZE: cppOut << "static_cast<usize>(" << jsonGet(cppOut, field.name) << ".intNumber())"; break;
							case FieldTypeType::CPP: cppOut << field.type << "::fromJson(" << jsonGet(cppOut, field.name) << ")"; break;
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

			//for (const auto& field : structure.fields) {
			//	std::stringstream signature;
			//	signature << "set";
			//	outFirstLetterUppercase(signature, field.name);
			//	signature << "(" << field.type << " value, const Entity& entity, Commands& commands" << ") -> void";

			//	hppOut << "\tauto " << signature.str() << ";\n"; 

			//	cppOut << "auto " << structure.name << "Editor::" << signature.str() << " {\n";
			//	cppOut << "\tcommands.addSetFieldCommand(entity, ";
			//	outOffsetName(cppOut, structure.name, field.name);
			//	cppOut << ", &" << field.name << ", &value, sizeof(" << field.name << "));\n";
			//	cppOut << "\t" << field.name << " = value;\n";
			//	cppOut << "}\n";
			//}


			hppOut << "};\n\n";

			// If offsetof is inside the class it is referencing it results in an error.
			for (const auto& field : structure.fields) {
				hppOut << "static constexpr auto ";
				outOffsetName(hppOut, structure.name, field.name);
				hppOut << " = offsetof(" << structure.name << "Editor, " << field.name << ");\n";
			}
			hppOut << '\n';



			hppOut << "struct " << structure.name << " : public " << structure.name << "Editor {\n";
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