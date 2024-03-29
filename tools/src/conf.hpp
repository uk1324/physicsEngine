#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace Data {

enum class FieldPropertyType {
	NO_SERIALIZE,
	CUSTOM,
};

struct FieldProperty {
	FieldPropertyType type;
	std::string_view customSerializeFn;
	std::string_view customDeserializeFn;
};

enum class FieldTypeType {
	FLOAT,
	I32,
	VEC2,
	VEC3,
	USIZE,
	CPP,
	ANGLE,
	VARIANT,
	VECTOR,
	BOOL,
	STRING,
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
		std::unique_ptr<FieldType> vectorType;
	};
};

struct Field {
	FieldType type;
	std::string_view name;
	std::vector<FieldProperty> properties;
	std::optional<std::string_view> defaultValueCpp;

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