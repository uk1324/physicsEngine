#include <confParser.hpp>
#include <asserts.hpp>
#include <iostream>

auto trimString(std::string_view line) -> std::string_view {
	static const char* WHITESPACE = "\t\n\r\f\v";

	auto prefixOffset = line.find_first_not_of(WHITESPACE);
	if (prefixOffset == std::string_view::npos)
		return line.substr(0, 0);

	line.remove_prefix(prefixOffset);

	auto suffixOffset = line.find_last_not_of(WHITESPACE);
	if (suffixOffset == std::string_view::npos)
		return line.substr(0, 0);

	line.remove_suffix(line.size() - suffixOffset);

	return line;
}

using namespace Data;

auto Data::tokenTypeToString(TokenType type) -> std::string_view {
	switch (type) {
	case TokenType::IDENTIFIER: return "identifier";
	case TokenType::LEFT_BRACE: return "'{'";
	case TokenType::RIGHT_BRACE: return "'}'";
	case TokenType::HASH: return "'#'";
	case TokenType::COLON: return "';'";
	case TokenType::CPP: return "cpp code";
	//case TokenType::ERROR: return "')'";
	case TokenType::AT: return "'@'";
	case TokenType::COMMA: return "','";
	case TokenType::SEMICOLON: return "';'";
	case TokenType::CARET: return "'^'";
	case TokenType::LEFT_PAREN: return "'('";
	case TokenType::RIGHT_PAREN: return "')'";
	}
	ASSERT_NOT_REACHED();
	return "";
}

Token::Token() 
	: type{ TokenType::ERROR }
	, start{ 0 }
	, length{ 0 } {}

Token::Token(TokenType type, usize start, usize length)
	: type{ type }
	, start{ start }
	, length{ length } {}

auto Scanner::init(std::string_view text) -> void {
	ASSERT(text.size() != 0);
	this->text = text;
	currentIndex = 0;
	tokenStartIndex = 0;
}

auto Scanner::nextToken() -> std::optional<Token> {
	if (text.size() == 0)
		return std::nullopt;

	skipWhitespace();

	if (isAtEnd())
		return std::nullopt;

	const auto c = peek(); consume();
	switch (c) {
	case '{': return makeToken(TokenType::LEFT_BRACE);
	case '}': return makeToken(TokenType::RIGHT_BRACE);
	case '#': return makeToken(TokenType::HASH);
	case '@': return makeToken(TokenType::AT);
	case ';': return makeToken(TokenType::SEMICOLON);
	case '^': return makeToken(TokenType::CARET);
	case '(': return makeToken(TokenType::LEFT_PAREN);
	case ')': return makeToken(TokenType::RIGHT_PAREN);
	case '~': {
		while (isAtEnd() == false && !match('~'))
			consume();

		if (isAtEnd())
			return errorToken("unterminated '~'");

		Token token = makeToken(TokenType::CPP);
		token.cpp = trimString(text.substr(token.start + 1, token.length - 1));
		return token;
	}

	default:
		auto isIdentifierStartChar = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_') || (c == '$'); };
		auto isDigit = [](char c) { return (c >= '0') && (c <= '9'); };

		if (isIdentifierStartChar(c)) {
			while (!isAtEnd() && (isIdentifierStartChar(peek()) || isDigit(peek())))
				consume();

			Token token = makeToken(TokenType::IDENTIFIER);
			token.identifier = text.substr(token.start, token.length);
			return token;
		}

		return errorToken("illegal character");
	}
}

auto Scanner::makeToken(TokenType type) -> Token {
	const Token token{ type, tokenStartIndex, currentIndex - tokenStartIndex };
	tokenStartIndex = currentIndex;
	return token;
}

auto Scanner::errorToken(std::string_view message) -> Token {
	std::cerr << message << '\n';
	return makeToken(TokenType::ERROR);
}

auto Scanner::skipWhitespace() -> void {
	while (!isAtEnd()) {
		switch (peek()) {
		case ' ':
		case '\t':
		case '\r':
		case '\f':
		case '\n':
			consume();
			break;
		case '/':
			if (peekNext() == '/') {
				while (!isAtEnd() && peek() != '\n')
					consume();
			}
			break;

		default:
			tokenStartIndex = currentIndex;
			return;
		}
	}
}

auto Scanner::peek() -> char {
	return text[currentIndex];
}

auto Scanner::match(char c) -> bool {
	if (peek() == c) {
		consume();
		return true;
	}
	return false;
}

auto Scanner::peekNext() -> char {
	if (currentIndex + 1 >= text.size())
		return '\0';
	return text[currentIndex + 1];
}

auto Scanner::consume() -> void {
	if (!isAtEnd())
		currentIndex++;
}

auto Scanner::isAtEnd() -> bool {
	return currentIndex >= text.size();
}

auto Parser::parse(std::string_view text) -> std::optional<DataFile> {
	DataFile output;
	if (text.empty())
		return output;

	isAtEnd = false;
	scanner.init(text);	
	auto token = scanner.nextToken();
	if (!token.has_value())
		return output;

	currentToken = std::move(*token);
	while (!isAtEnd) {
		try {
			if (match(TokenType::IDENTIFIER)) {
				const auto structName = previousToken.identifier;
				std::vector<StructPropertyType> properties;
				if (match(TokenType::AT)) {
					while (!isAtEnd && currentToken.type != TokenType::LEFT_BRACE) {
						if (matchIdentifier("ImGui")) {
							properties.push_back(StructPropertyType::IM_GUI);
						} else if (matchIdentifier("Editor")) {
							properties.push_back(StructPropertyType::IM_GUI);
							properties.push_back(StructPropertyType::SERIALIZABLE);
						} else if (match(TokenType::IDENTIFIER)) {
							std::cerr << "unknown property " << previousToken.identifier << "ignored";
						} else {
							std::cerr << "expected property name";
							throw Error{};
						}
					}
				}

				Struct structure{ .name = structName, .properties = std::move(properties) };

				expect(TokenType::LEFT_BRACE);
				while (!isAtEnd && !match(TokenType::RIGHT_BRACE)) {
					// Initializers should use the CPP token
					if (match(TokenType::CARET)) {
						expect(TokenType::CPP);
						structure.cppCode.push_back(previousToken.cpp);
					} else {
						const auto type = fieldType();
						expect(TokenType::IDENTIFIER);
						const auto name = previousToken.identifier;
						std::vector<FieldProperty> fieldProperties;
						while (!isAtEnd && currentToken.type != TokenType::SEMICOLON) {
							expect(TokenType::IDENTIFIER);
							const auto propertyName = previousToken.identifier;

							auto parseArgs = [&](i32 count) -> std::vector<std::string_view> {
								std::vector<std::string_view> result;
								expect(TokenType::LEFT_PAREN);
								for (i32 i = 0; i < count; i++) {
									expect(TokenType::IDENTIFIER);
									result.push_back(previousToken.identifier);
								}
								expect(TokenType::RIGHT_PAREN);
								return result;
							};

							if (propertyName == "Custom") {
								const auto args = parseArgs(3);
								FieldProperty property{ .type = FieldPropertyType::CUSTOM };
								property.customSerializeFn = args[0];
								property.customDeserializeFn = args[1];
								property.customGuiFn = args[2];
								fieldProperties.push_back(property);
							} else {
								std::cerr << "invalid property '" << propertyName << "' ignored";
							}
						}
						expect(TokenType::SEMICOLON);
						structure.fields.push_back(Field{ .type = type, .name = name, .properties = std::move(fieldProperties) });
					}
				}

				output.structs.push_back(std::move(structure));
			} else if (match(TokenType::CPP)) {
				output.cppCode.push_back(previousToken.cpp);
			}
		} catch (const Error&) {
			output.~DataFile();
			new (&output) DataFile();
			return std::nullopt;
		}
	}
	return output;
}

auto Parser::fieldType() -> FieldType {
	if (match(TokenType::CPP)) {
		FieldType type{ FieldTypeType::CPP };
		type.cpp = previousToken.cpp;
		return type;
	} 
	else if (matchIdentifier("float")) return FieldType{ FieldTypeType::FLOAT };
	else if (matchIdentifier("angle")) return FieldType{ FieldTypeType::ANGLE };
	else if (matchIdentifier("i32")) return FieldType{ FieldTypeType::I32 };
	else if (matchIdentifier("Vec2")) return FieldType{ FieldTypeType::VEC2 };
	else {
		std::cerr << "expected type\n";
		throw Error{};
	}
}

auto Parser::consume() -> void {
	if (isAtEnd)
		return;

	auto next = scanner.nextToken();
	if (next.has_value() == false) {
		isAtEnd = true;
		return;
	}
	previousToken = std::move(currentToken);
	currentToken = std::move(*next);
	//std::cout << currentToken.start << ' ' << currentToken.length << '\n' << scanner.text.substr(currentToken.start, currentToken.length) << '\n';
}

auto Parser::match(TokenType type) -> bool {
	if (currentToken.type == type) {
		consume();
		return true;
	}
	return false;
}

auto Parser::matchIdentifier(std::string_view text) -> bool {
	if (currentToken.type == TokenType::IDENTIFIER && currentToken.identifier == text) {
		consume(); 
		return true;
	}
	return false;
}

auto Parser::expect(TokenType type) -> void {
	if (!match(type) || isAtEnd) {
		std::cerr << "expected " << tokenTypeToString(type) << '\n';
		throw Error{};
	}
}