#include <confParser.hpp>
#include <asserts.hpp>
#include <iostream>

using namespace Conf;

auto Conf::tokenTypeToString(TokenType type) -> std::string_view {
	switch (type) {
	case TokenType::IDENTIFIER: return "identifier";
	case TokenType::LEFT_BRACE: return "'{'";
	case TokenType::RIGHT_BRACE: return "'}'";
	case TokenType::HASH: return "'#'";
	case TokenType::COLON: return "';'";
	}
	ASSERT_NOT_REACHED();
}

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
	case '~': {
		while (isAtEnd() == false && !match('~'))
			consume();

		if (isAtEnd())
			return errorToken("unterminated '~'");

		consume();
		Token token = makeToken(TokenType::CPP);
		token.identifier = text.substr(token.start + 1, token.length - 1);
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

auto Parser::parse(std::string_view text) -> std::optional<ConfigFile> {
	isAtEnd = false;
	scanner.init(text);
	ConfigFile output;
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
					const auto type = fieldType();
					expect(TokenType::IDENTIFIER);
					const auto name = previousToken.identifier;
					structure.fields.push_back(Field{ .type = type, .name = name });
					expect(TokenType::SEMICOLON);
				}

				output.structs.push_back(std::move(structure));
			} else if (match(TokenType::CPP)) {
				output.cppCode.push_back(previousToken.cpp);
			}
		} catch (const Error&) {
			output.~ConfigFile();
			new (&output) ConfigFile();
			return std::nullopt;
		}
	}
	return output;
}

auto Parser::fieldType() -> FieldType {
	if (match(TokenType::CPP)) {
		FieldType type{ FieldTypeType::FLOAT };
		type.cpp = previousToken.cpp;
		return type;
	} else if (matchIdentifier("float")) {
		return FieldType{ FieldTypeType::FLOAT };
	} else if (matchIdentifier("i32")) {
		return FieldType{ FieldTypeType::I32 };
	} else {
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
