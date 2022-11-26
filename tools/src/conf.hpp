#pragma once

#include <string>
#include <vector>

namespace Conf {

enum class FieldPropertyType {
	NO_SERIALIZE
};

struct FieldProperty {
	FieldPropertyType type;
};

enum class FieldTypeType {
	FLOAT,
	I32,
	VEC2,
	CPP
};

struct FieldType {
	FieldTypeType type;

	union {
		std::string_view cpp;
	};
};

struct Field {
	FieldType type;
	std::string_view name;
	std::vector<FieldProperty> properties;
};

enum class StructPropertyType {
	IM_GUI
};

struct Struct {
	std::string_view name;
	std::vector<Field> fields;
	std::vector<StructPropertyType> properties;
};

struct ConfigFile {
	std::vector<std::string_view> cppCode;
	std::vector<Struct> structs;
};

}