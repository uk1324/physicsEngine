#pragma once

#include <string>
#include <vector>

namespace Data {

enum class FieldPropertyType {
	NO_SERIALIZE,
	CUSTOM,
};

struct FieldProperty {
	FieldPropertyType type;
	std::string_view customSerializeFn;
	std::string_view customDeserializeFn;
	std::string_view customGuiFn;
	//bool serializeFnIsMethod;
};

enum class FieldTypeType {
	FLOAT,
	I32,
	VEC2,
	USIZE,
	CPP,
	ANGLE,
	VARIANT,
};

struct FieldType {
	FieldTypeType type;
	FieldType(FieldTypeType type);
	FieldType(const FieldType& type);
	auto operator=(const FieldType& type) -> FieldType&;
	~FieldType();

	union {
		std::string_view cpp;
		std::vector<FieldType> variant;
	};
};

struct Field {
	FieldType type;
	std::string_view name;
	std::vector<FieldProperty> properties;
};

enum class StructPropertyType {
	IM_GUI,
	SERIALIZABLE
};

struct Struct {
	std::string_view name;
	std::vector<Field> fields;
	std::vector<StructPropertyType> properties;
	std::vector<std::string_view> cppCode;
};

struct DataFile {
	std::vector<std::string_view> cppCode;
	std::vector<Struct> structs;
	enum struct CodeType {
		CPP_CODE,
		STRUCT,
	};
	struct Code {
		CodeType type;
		size_t index;
	};
	std::vector<Code> orderedCode;
};

}