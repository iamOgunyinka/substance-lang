/**************************************************************************
 * Language scanner
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * All rights reserved.
 */

#include "scanner.h"

using namespace compiler;

#define EOB L'\0'

/****************************
 * Scanner constructor
 ****************************/

using std::wstring;

Scanner::Scanner( const wstring &input, bool is_file )
{
	// create tokens
	for ( int i = 0; i < LOOK_AHEAD; i++ ) {
		tokens[ i ] = new Token;
	}
	// load identifiers into map
	LoadKeywords();

	if ( is_file ) {
		file_name = input;
		ReadFile( input );
	}
	else {
		ReadLine( input );
	}
	line_num = 1;
}

/****************************
 * Scanner destructor
 ****************************/
Scanner::~Scanner()
{
	// delete buffer
	if ( buffer ) {
		delete [] buffer;
		buffer = NULL;
	}

	for ( int i = 0; i < LOOK_AHEAD; i++ ) {
		Token* temp = tokens[ i ];
		delete temp;
		temp = NULL;
	}
}

/****************************
 * Loads language keywords
 ****************************/
void Scanner::LoadKeywords()
{
	ident_map[ L"var" ] = ScannerTokenType::TOKEN_VAR_ID;
	ident_map[ L"const" ] = ScannerTokenType::TOKEN_CONST_ID;
	ident_map[ L"if" ] = ScannerTokenType::TOKEN_IF_ID;
	ident_map[ L"else" ] = ScannerTokenType::TOKEN_ELSE_ID;
	ident_map[ L"switch" ] = ScannerTokenType::TOKEN_SWITCH_ID;
	ident_map[ L"case" ] = ScannerTokenType::TOKEN_CASE_ID;
	ident_map[ L"do" ] = ScannerTokenType::TOKEN_DO_ID;
	ident_map[ L"while" ] = ScannerTokenType::TOKEN_WHILE_ID;
	ident_map[ L"for" ] = ScannerTokenType::TOKEN_FOR_ID;
	ident_map[ L"foreach" ] = ScannerTokenType::TOKEN_FOR_EACH_ID;
	ident_map[ L"each" ] = ScannerTokenType::TOKEN_EACH_ID;
	ident_map[ L"in" ] = ScannerTokenType::TOKEN_IN_ID;
	ident_map[ L"of" ] = ScannerTokenType::TOKEN_OF_ID;
	ident_map[ L"show" ] = ScannerTokenType::TOKEN_SHOW_ID;
	ident_map[ L"self" ] = ScannerTokenType::TOKEN_SELF_ID;
	ident_map[ L"return" ] = ScannerTokenType::TOKEN_RETURN_ID;
	ident_map[ L"break" ] = ScannerTokenType::TOKEN_BREAK_ID;
	ident_map[ L"continue" ] = ScannerTokenType::TOKEN_CONTINUE_ID;
	ident_map[ L"class" ] = ScannerTokenType::TOKEN_CLASS_ID;
	ident_map[ L"struct" ] = ScannerTokenType::TOKEN_STRUCT_ID;
	ident_map[ L"construct" ] = ScannerTokenType::TOKEN_CONSTRUCT_ID;
	ident_map[ L"function" ] = ScannerTokenType::TOKEN_FUNC_ID;
	ident_map[ L"method" ] = ScannerTokenType::TOKEN_METHOD_ID;
	ident_map[ L"public" ] = ScannerTokenType::TOKEN_PUBLIC_ID;
	ident_map[ L"private" ] = ScannerTokenType::TOKEN_PRIVATE_ID;
	ident_map[ L"protected" ] = ScannerTokenType::TOKEN_PROTECTED_ID;
	ident_map[ L"static" ] = ScannerTokenType::TOKEN_STATIC_ID;
	ident_map[ L"true" ] = ScannerTokenType::TOKEN_TRUE_LIT;
	ident_map[ L"false" ] = ScannerTokenType::TOKEN_FALSE_LIT;
	ident_map[ L"new" ] = ScannerTokenType::TOKEN_NEW;
	ident_map[ L"null" ] = ScannerTokenType::TOKEN_NULL;
	ident_map[ L"block" ] = ScannerTokenType::TOKEN_BLOCK;
	ident_map[ L"extern" ] = ScannerTokenType::TOKEN_EXTERN_ID;
	ident_map[ L"loop" ] = ScannerTokenType::TOKEN_LOOP_ID;
}

/****************************
 * Processes language
 * identifiers
 ****************************/
void Scanner::CheckIdentifier( int index )
{
	// copy wstring
	const int length = end_pos - start_pos;
	wstring ident( buffer, start_pos, length );

	// check wstring
	auto ident_find = ident_map.find( ident );
	ScannerTokenType ident_type = ( ident_find != ident_map.end() ) ? ident_find->second : ScannerTokenType::TOKEN_IDENT;
	tokens[ index ]->SetType( ident_type );
	tokens[ index ]->SetLineNbr( line_num );
	tokens[ index ]->SetFileName( file_name );

	if ( ident_find == ident_map.end() ){ // we have a identifier
		tokens[ index ]->SetIdentifier( ident );
	}
}

/****************************
 * Reads a source input file
 ****************************/
void Scanner::ReadLine( const wstring &line )
{
	buffer_pos = 0;
	buffer = new wchar_t[ line.size() + 1 ];
#ifdef _WIN32
	wcsncpy_s( buffer, line.size(), line.c_str(), _TRUNCATE );
#else
	wcsncpy( buffer, line.c_str(), line.size() );
#endif
	buffer[ line.size() ] = '\0';
	buffer_size = line.size() + 1;
#ifdef _DEBUG
	std::wcout << L"---------- Source ---------" << std::endl;
	std::wcout << buffer << std::endl;
#endif
}

/****************************
 * Reads a source input file
 ****************************/
void Scanner::ReadFile( const wstring &name )
{
	buffer_pos = 0;
	buffer = LoadFileBuffer( name, buffer_size );

#ifdef _DEBUG
	std::wcout << L"---------- Source ---------" << std::endl;
	std::wcout << buffer << std::endl;
#endif
}

/****************************
 * Processes the next token
 ****************************/
void Scanner::NextToken()
{
	if ( buffer_pos == 0 ) {
		NextChar();
		for ( int i = 0; i < LOOK_AHEAD; i++ ) {
			ParseToken( i );
		}
	}
	else {
		int i = 1;
		for ( ; i < LOOK_AHEAD; i++ ) {
			*( tokens[ i - 1 ] ) = *( tokens[ i ] );
		}
		ParseToken( i - 1 );
	}
}

/****************************
 * Gets the current token
 ****************************/
Token* Scanner::GetToken( int index )
{
	if ( index < LOOK_AHEAD ) {
		return tokens[ index ];
	}

	return nullptr;
}

/****************************
 * Gets the next character.
 * Note, EOB is returned at
 * end of a stream
 ****************************/
void Scanner::NextChar()
{
	if ( buffer_pos < buffer_size ) {
		// line number
		if ( cur_char == L'\n' ) {
			line_num++;
		}
		// current character    
		cur_char = buffer[ buffer_pos++ ];
		// next character
		if ( buffer_pos < buffer_size ) {
			nxt_char = buffer[ buffer_pos ];
			// next next character
			if ( buffer_pos + 1 < buffer_size ) {
				nxt_nxt_char = buffer[ buffer_pos + 1 ];
			}
			// end of file
			else {
				nxt_nxt_char = EOB;
			}
		}
		// end of file
		else {
			nxt_char = EOB;
		}
	}
	// end of file
	else {
		cur_char = EOB;
	}
}

/****************************
 * Processes white space
 ****************************/
void Scanner::Whitespace()
{
	while ( WHITE_SPACE && cur_char != EOB ) {
		NextChar();
	}
}

/****************************
 * Parses a token
 ****************************/
void Scanner::ParseToken( int index )
{
	// unable to load buffer
	if ( !buffer ) {
		tokens[ index ]->SetType( ScannerTokenType::TOKEN_NO_INPUT );
		tokens[ index ]->SetLineNbr( line_num );
		tokens[ index ]->SetFileName( file_name );
		return;
	}
	// ignore white space
	Whitespace();

	// ignore comments
	if ( cur_char == COMMENT && ( nxt_char == COMMENT || nxt_char == EXTENDED_COMMENT ) ) {
		while ( cur_char == COMMENT && cur_char != EOB ) {
			NextChar();
			// extended comment
			if ( cur_char == EXTENDED_COMMENT ) {
				NextChar();
				while ( !( cur_char == EXTENDED_COMMENT && nxt_char == COMMENT ) && cur_char != EOB ) {
					NextChar();
				}
				NextChar();
				NextChar();
			}
			// line comment
			else if ( cur_char == COMMENT ) {
				while ( cur_char != L'\n' && cur_char != EOB ) {
					NextChar();
				}
			}
			Whitespace();
		}
	}

	// character wstring
	if ( cur_char == L'\"' ) {
		NextChar();
		// mark
		start_pos = ( int ) buffer_pos - 1;
		while ( cur_char != L'\"' && cur_char != EOB ) {
			if ( cur_char == L'\\' ) {
				NextChar();
				switch ( cur_char ) {
				case L'"':
					break;

				case L'\\':
					break;

				case L'n':
					break;

				case L'r':
					break;

				case L't':
					break;

				case L'0':
					break;

				default:
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_UNKNOWN );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					break;
				}
			}
			NextChar();
		}
		// mark
		end_pos = ( int ) buffer_pos - 1;
		// check wstring
		NextChar();
		CheckString( index );
		return;
	}
	// character
	else if ( cur_char == L'\'' ) {
		NextChar();
		// escape or hex/unicode encoding
		if ( cur_char == L'\\' ) {
			NextChar();
			// read unicode string
			if ( cur_char == L'u' ) {
				NextChar();
				start_pos = ( int ) buffer_pos - 1;
				while ( iswdigit( cur_char ) || ( cur_char >= L'a' && cur_char <= L'f' ) ||
					( cur_char >= L'A' && cur_char <= L'F' ) ) {
					NextChar();
				}
				end_pos = ( int ) buffer_pos - 1;
				ParseUnicodeChar( index );
				if ( cur_char != L'\'' ) {
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_UNKNOWN );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
				}
				NextChar();
				return;
			}
			// escape
			else if ( nxt_char == L'\'' ) {
				switch ( cur_char ) {
				case L'n':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\n' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;

				case L'r':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\r' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;

				case L't':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\t' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;

				case L'a':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\a' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;

				case L'b':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\b' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;

				case L'f':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\f' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;

				case L'\\':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\\' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;

				case L'\'':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\'' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;

				case L'0':
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
					tokens[ index ]->SetCharLit( L'\0' );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					NextChar();
					return;
				}
			}
			// error
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_UNKNOWN );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
				NextChar();
				return;
			}
		}
		// error
		else {
			if ( nxt_char != L'\'' ) {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_UNKNOWN );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
				NextChar();
				return;
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_CHAR_LIT );
				tokens[ index ]->SetCharLit( cur_char );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
				NextChar();
				NextChar();
				return;
			}
		}
	}
	// identifier
	else if ( isalpha( cur_char ) /* || cur_char == L'@' || cur_char == L'?' */|| cur_char == L'_' ) {
		// mark
		start_pos = ( int ) buffer_pos - 1;
		while ( ( isalpha( cur_char ) || isdigit( cur_char ) || cur_char == L'_' ) && cur_char != EOB ) {
			NextChar();
		}
		// mark
		end_pos = ( int ) buffer_pos - 1;
		// check identifier
		CheckIdentifier( index );
		return;
	}
	// number
	else if ( iswdigit( cur_char ) || ( cur_char == L'.' && iswdigit( nxt_char ) ) ) {
		bool is_double = false;
		int hex_state = 0;
		// mark
		start_pos = ( int ) buffer_pos - 1;

		// test hex state
		if ( cur_char == L'0' ) {
			hex_state = 1;
		}
		while ( iswdigit( cur_char ) || ( cur_char == L'.' && iswdigit( nxt_char ) ) || cur_char == L'x' ||
			( cur_char >= L'a' && cur_char <= L'f' ) ||
			( cur_char >= L'A' && cur_char <= L'F' ) ) {
			// decimal double
			if ( cur_char == L'.' ) {
				// error
				if ( is_double ) {
					tokens[ index ]->SetType( ScannerTokenType::TOKEN_UNKNOWN );
					tokens[ index ]->SetLineNbr( line_num );
					tokens[ index ]->SetFileName( file_name );
					NextChar();
					break;
				}
				is_double = true;
			}
			// hex integer
			if ( cur_char == L'x' ) {
				if ( hex_state == 1 ) {
					hex_state = 2;
				}
				else {
					hex_state = 1;
				}
			}
			else {
				hex_state = 0;
			}
			// next character
			NextChar();
		}
		// mark
		end_pos = ( int ) buffer_pos - 1;
		if ( is_double ) {
			ParseDouble( index );
		}
		else if ( hex_state == 2 ) {
			ParseInteger( index, 16 );
		}
		else if ( hex_state ) {
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_UNKNOWN );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
		}
		else {
			ParseInteger( index );
		}
		return;
	}
	// other
	else {
		switch ( cur_char ) {
		case L':':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_COLON );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'-':
			if ( nxt_char == L'>' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_ASSESSOR );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
				NextChar();
			}
			else if ( nxt_char == L'=' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_SUB_EQL );
			}
			else if ( nxt_char == L'-' ){
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_DECR );
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_SUB );
			}
			tokens[ index ]->SetFileName( file_name );
			tokens[ index ]->SetLineNbr( line_num );
			NextChar();
			break;

		case L'{':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_OPEN_BRACE );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'}':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_CLOSED_BRACE );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'.':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_PERIOD );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'[':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_OPEN_BRACKET );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L']':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_CLOSED_BRACKET );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'(':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_OPEN_PAREN );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L')':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_CLOSED_PAREN );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L',':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_COMMA );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L';':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_SEMI_COLON );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'&':
			if ( nxt_char == L'&' ){
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_LAND );
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_AND );
			}
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;
		case L'?':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_QUESTION_MARK );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;
		case L'|':
			if ( nxt_char == L'|' ){
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_LOR );
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_OR );
			}
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'=':
			if ( nxt_char == L'=' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_EQL );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
				NextChar();
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_ASSIGN );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
				NextChar();
			}
			break;

		case L'!':
			if ( nxt_char == L'=' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_NEQL );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_NOT );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
			}
			NextChar();
			break;

		case L'<':
			if ( nxt_char == L'=' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_LEQL );
			}
			else if ( nxt_char == L'<' ){
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_LSHIFT );
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_LES );
			}
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'>':
			if ( nxt_char == L'=' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_GEQL );
				NextChar();
			}
			else if ( nxt_char == L'>' ){
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_RSHIFT );
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_GTR );
			}
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'+':
			if ( nxt_char == L'=' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_ADD_EQL );
			}
			else if ( nxt_char == L'+' ){
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_INCR );
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_ADD );
			}
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'*':
			if ( nxt_char == L'=' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_MUL_EQL );
			}
			else if ( nxt_char == L'*' ){
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_EXP );
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_MUL );
			}
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;

		case L'/':
			if ( nxt_char == L'=' ) {
				NextChar();
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_DIV_EQL );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
				NextChar();
			}
			else {
				tokens[ index ]->SetType( ScannerTokenType::TOKEN_DIV );
				tokens[ index ]->SetLineNbr( line_num );
				tokens[ index ]->SetFileName( file_name );
				NextChar();
			}
			break;
		case L'%':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_MOD );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;
		case L'^':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_XOR );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;
		case L'@':
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_AT );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;
		case EOB:
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_END_OF_STREAM );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			break;

		default:
			ProcessWarning();
			tokens[ index ]->SetType( ScannerTokenType::TOKEN_UNKNOWN );
			tokens[ index ]->SetLineNbr( line_num );
			tokens[ index ]->SetFileName( file_name );
			NextChar();
			break;
		}
		return;
	}
}
