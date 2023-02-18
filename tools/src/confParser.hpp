#pragma once

#include "../../src/utils/int.hpp"
#include <conf.hpp>
#include <vector>
#include <optional>

auto trimString(std::string_view line) -> std::string_view;

namespace Data {

enum class TokenType {
	IDENTIFIER,
	LEFT_BRACE,
	RIGHT_BRACE,
	HASH,
	COLON,
	CPP,
	ERROR,
	AT,
	COMMA,
	SEMICOLON,
	CARET,
	LEFT_PAREN,
	RIGHT_PAREN,
	LESS_THAN,
	MORE_THAN,
	EQUALS,
};

auto tokenTypeToString(TokenType type) -> std::string_view;

struct Token {
	Token();
	Token(TokenType type, usize start, usize length);

	TokenType type;

	union {
		std::string_view identifier;
		std::string_view cpp;
	};
	
	usize start;
	usize length;
};

class Scanner {
public:
	auto init(std::string_view text) -> void;
	auto nextToken() -> std::optional<Token>;

private:
	auto makeToken(TokenType type) -> Token;
	auto errorToken(std::string_view message) -> Token;
	auto skipWhitespace() -> void;
	auto peek() -> char;
	auto match(char c) -> bool;
	auto peekNext() -> char;
	auto consume() -> void;
	auto isAtEnd() -> bool;

public:
	std::string_view text;
private:
	usize tokenStartIndex = 0;
	usize currentIndex = 0;
};

class Parser {
public:
	auto parse(std::string_view text) -> std::optional<DataFile>;

private:
	auto fieldType() -> FieldType;

	struct Error {};

	auto consume() -> void;
	auto match(TokenType type) -> bool;
	auto matchIdentifier(std::string_view text) -> bool;
	// !!!! TODO: For error create a function errorLocation() that can return a type that can be printed out.
	auto expect(TokenType type) -> void;

	Token previousToken;
	Token currentToken;

	bool isAtEnd = false;

	Scanner scanner;
};

}