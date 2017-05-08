/***************************************************************************
 * Language scanner
 * 
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
 */

#ifndef __SCANNER_H__
#define __SCANNER_H__

#include "common.h"

// comment
#define COMMENT L'/'
#define EXTENDED_COMMENT L'*'

// look ahead value
#define LOOK_AHEAD 3
// white space
#define WHITE_SPACE (cur_char == L' ' || cur_char == L'\t' || cur_char == L'\r' || cur_char == L'\n')

namespace compiler {
	/****************************
	 * Token types
	 ****************************/
	enum class ScannerTokenType {
		// misc
		TOKEN_END_OF_STREAM = -1000,
		TOKEN_NO_INPUT,
		TOKEN_UNKNOWN,
		// symbols
		TOKEN_AT,
		TOKEN_QUESTION_MARK,
		TOKEN_PERIOD,
		TOKEN_COLON,
		TOKEN_SEMI_COLON,
		TOKEN_COMMA,
		TOKEN_ASSIGN,
		TOKEN_OPEN_BRACE,
		TOKEN_CLOSED_BRACE,
		TOKEN_OPEN_PAREN,
		TOKEN_CLOSED_PAREN,
		TOKEN_OPEN_BRACKET,
		TOKEN_CLOSED_BRACKET,
		TOKEN_ASSESSOR,
		// operations
		TOKEN_AND,
		TOKEN_OR,
		TOKEN_NOT,
		TOKEN_EQL,
		TOKEN_NEQL,
		TOKEN_LES,
		TOKEN_GTR,
		TOKEN_GEQL,
		TOKEN_LEQL,
		TOKEN_ADD,
		TOKEN_SUB,
		TOKEN_MUL,
		TOKEN_DIV,
		TOKEN_MOD,
		TOKEN_XOR,
		TOKEN_EXP, // **
		TOKEN_LAND, //logical AND &&
		TOKEN_LOR, // logical OR ||
		TOKEN_RSHIFT,
		TOKEN_LSHIFT,
		// compound operations
		TOKEN_ADD_EQL,
		TOKEN_SUB_EQL,
		TOKEN_MUL_EQL,
		TOKEN_DIV_EQL,
		TOKEN_INCR,
		TOKEN_DECR,
		// literals
		TOKEN_TRUE_LIT,
		TOKEN_FALSE_LIT,
		TOKEN_INT_LIT,
		TOKEN_FLOAT_LIT,
		TOKEN_CHAR_LIT,
		TOKEN_CHAR_STRING_LIT,
		TOKEN_NEW,
		TOKEN_NULL,
		// types and modifiers
		TOKEN_FUNC_ID, // start keywords
		TOKEN_CONSTRUCT_ID,
		TOKEN_METHOD_ID,
		TOKEN_IDENT,
		TOKEN_VAR_ID,
		TOKEN_CLASS_ID,
		TOKEN_PUBLIC_ID,
		TOKEN_PRIVATE_ID,
		TOKEN_PROTECTED_ID,
		TOKEN_STATIC_ID,
		// control
		TOKEN_IF_ID,
		TOKEN_ELSE_ID,
		TOKEN_SWITCH_ID,
		TOKEN_CASE_ID,
		TOKEN_DO_ID,
		TOKEN_WHILE_ID,
		TOKEN_FOR_ID,
		TOKEN_EACH_ID,
		TOKEN_IN_ID,
		TOKEN_FOR_EACH_ID,
		TOKEN_OF_ID,
		TOKEN_SHOW_ID,
		TOKEN_SELF_ID,
		TOKEN_BREAK_ID,
		TOKEN_CONTINUE_ID,
		TOKEN_RETURN_ID // end keywords
	};

	/****************************
	 * Token class
	 ****************************/
	class Token {
		ScannerTokenType	token_type;
		std::wstring		ident;
		unsigned int		line_num;
		std::wstring		file_name;

		INT_T				int_lit;
		FLOAT_T				double_lit;
		CHAR_T				char_lit;
		BYTE_T				byte_lit;
	public:
		inline const std::wstring GetFileName() const {
			return file_name;
		}

		inline void SetFileName( std::wstring f ) {
			file_name = f;
		}

		inline const unsigned int GetLineNumber() const {
			return line_num;
		}

		inline void SetLineNbr( unsigned int l ) {
			line_num = l;
		}

		inline void  SetIntLit( INT_T i ) {
			int_lit = i;
		}

		inline void SetFloatLit( FLOAT_T d ) {
			double_lit = d;
		}

		inline void SetByteLit( BYTE_T b ) {
			byte_lit = b;
		}

		inline void SetCharLit( CHAR_T c ) {
			char_lit = c;
		}

		inline void SetIdentifier( std::wstring i ) {
			ident = i;
		}

		inline const INT_T GetIntLit() const {
			return int_lit;
		}

		inline const FLOAT_T GetFloatLit() const {
			return double_lit;
		}

		inline const BYTE_T GetByteLit() const {
			return byte_lit;
		}

		inline const CHAR_T GetCharLit() const {
			return char_lit;
		}

		inline const std::wstring GetIdentifier() const {
			return ident;
		}

		inline const ScannerTokenType GetType() const {
			return token_type;
		}

		inline void SetType( ScannerTokenType t ) {
			token_type = t;
		}
	};

	/***************************************
	 * Token scanner with k lookahead tokens
	 **************************************/
	class Scanner {
	private:
		std::wstring	file_name;		// input file name
		unsigned int	line_num;		// line number
		wchar_t*		buffer;			// input buffer
		size_t			buffer_size;	// buffer size
		size_t			buffer_pos;		// input buffer position
		int				start_pos;		// start marker position
		int				end_pos;		// end marker position
		wchar_t			cur_char;
		wchar_t			nxt_char;
		wchar_t			nxt_nxt_char;	// input characters
		std::map<std::wstring const, ScannerTokenType> ident_map; // map of reserved identifiers
		Token* tokens[ LOOK_AHEAD ];	// array of tokens for lookahead
		

		// warning message
		void ProcessWarning() {
			std::wcout << L"Parse warning: Unknown token: '" << cur_char << L"'" << std::endl;
		}

		// parsers a character wstring
		inline void CheckString( int index ) {
			// copy wstring
			const int length = end_pos - start_pos;
			std::wstring char_string( buffer, start_pos, length );
			// set wstring
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_STRING_LIT );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetIdentifier( char_string );
			tokens[ index ]->SetFileName( file_name );
		}

		// parse an integer
		inline void ParseInteger( int index, int base = 0 ) {
			// copy wstring
			int length = end_pos - start_pos;
			std::wstring ident( buffer, start_pos, length );

			// set token
			wchar_t* end;
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_INT_LIT );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			tokens[ index ]->SetIntLit( wcstol( ident.c_str(), &end, base ) );
		}

		// parse a double
		inline void ParseDouble( int index ) {
			// copy wstring
			const int length = end_pos - start_pos;
			std::wstring wident( buffer, start_pos, length );
			// set token
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_FLOAT_LIT );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			
			std::string const ident( wident.begin(), wident.end() );
			tokens[ index ]->SetFloatLit( atof( ident.c_str() ) );
		}

		// parsers an unicode character
		inline void ParseUnicodeChar( int index ) {
			// copy wstring
			const int length = end_pos - start_pos;
			std::wstring ident( buffer, start_pos, length );
			// set token
			wchar_t* end;
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			tokens[ index ]->SetCharLit( ( wchar_t ) wcstol( ident.c_str(), &end, 16 ) );
		}


		// reads a file into memory
		void ReadFile( const std::wstring &name );
		// reads a line as input
		void ReadLine( const std::wstring &line );
		// ignore white space
		void Whitespace();
		// next character
		void NextChar();
		// load reserved keywords
		void LoadKeywords();
		// parses a new token
		void ParseToken( int index );
		// check identifier
		void CheckIdentifier( int index );

	public:
		// default constructor
		Scanner( const std::wstring &name, bool is_file = true );
		// default destructor
		~Scanner();

		// next token
		void NextToken();

		// token accessor
		Token* GetToken( int index = 0 );
	};
}

#endif
