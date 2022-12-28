#include <conf.hpp>

Data::FieldType::FieldType(FieldTypeType type)
	: type{ type } {}

Data::FieldType::FieldType(const FieldType& type)
	: type{ type.type } {
	switch (type.type) {
		case FieldTypeType::CPP: 
			cpp = type.cpp;
			break;
		case FieldTypeType::VARIANT:
			new (&variant) std::vector<FieldType>(type.variant);
			break;
		default:
			break;
	}
}

auto Data::FieldType::operator=(const FieldType& type) -> FieldType& {
	if (this->type == FieldTypeType::VARIANT) {
		variant.~vector();
	}

	this->type = type.type;

	switch (type.type) {
	case FieldTypeType::CPP:
		cpp = type.cpp;
		break;
	case FieldTypeType::VARIANT:
		new (&variant) std::vector<FieldType>(type.variant);
		break;
	default:
		break;
	}

	return *this;
}

Data::FieldType::~FieldType() {
	if (type == FieldTypeType::VARIANT) {
		variant.~vector();
	}
}
