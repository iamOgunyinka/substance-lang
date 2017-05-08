/***************************************************************************
 * Parse tree.
 * Copyright (c) 2017 Joshua Ogunyinka
 * Copyright (c) 2013-2016 Randy Hollines
 */

#include "tree.h"

using namespace compiler;

/****************************
* ParsedFunction class
****************************/
ParsedFunction::ParsedFunction( const std::wstring &file_name, const unsigned int line_num, const std::wstring &name,
	std::unique_ptr<ExpressionList> params ) : ParseNode( file_name, line_num ), name( name ),
	parameters( std::move( params ) )
{
	this->operation = NO_OP;
	this->local_count = 0;
	this->access = AccessType::PUBLIC_ACCESS;
	this->is_static = false;
}

ParsedFunction::ParsedFunction( const std::wstring &file_name, const unsigned int line_num, InstructionType operation,
	std::unique_ptr<ExpressionList> params ) : ParseNode( file_name, line_num ), name( L"" ),
	parameters( std::move( params ) )
{
	this->operation = operation;
	this->local_count = 0;
	this->access = AccessType::PUBLIC_ACCESS;
	this->is_static = false;
}

Scope* Scope::GetParentScope(){
	return parent;
}

void Scope::SetParentScope( Scope *scope )
{
	parent = scope;
}

void Scope::SetScopeType( ScopeType t )
{
	type = t;
}

template< template<typename> class Container, typename Element>
bool GenericAdder( std::wstring const & name, Container<Element> & container, Element && elem )
{

}

bool Scope::AddClass( std::unique_ptr<ParsedClass> klass ){
	std::wstring const class_name = klass->GetName();
	auto const found_iter = parsed_classes.find( class_name );
	if ( found_iter == parsed_classes.cend() ){
		parsed_classes.insert( { class_name, std::move( klass ) } );
		return true;
	}
	return false;
}

bool Scope::AddFunction( std::unique_ptr<ParsedFunction> function ){
	std::wstring const function_name = function->GetName();
	function_names.insert( function_name );
	return parsed_functions.insert( { function_name, std::move( function ) } ) != parsed_functions.cend();
}

void Scope::AddStatement( std::unique_ptr<Statement> statement ){
	statements.push_back( std::move( statement ) );
}

Scope::list_of_ptrs<Statement>& Scope::GetStatements(){
	return statements;
}

bool Scope::AddDeclaration( std::shared_ptr<Declaration> declaration ){
	std::wstring const variable_name = declaration->GetIdentifier();
	auto const found_iter = std::find_if( std::begin( declarations ), std::end( declarations ),
		[ &variable_name ] ( std::shared_ptr<Declaration> & decl ){ return decl->GetIdentifier() == variable_name; } );
	bool const not_found = found_iter == declarations.cend();
	if ( not_found ){
		if ( function_names.find( variable_name ) != function_names.end() ||
			parsed_classes.find( variable_name ) != parsed_classes.end() ) 
		{
			return false;
		}
		declarations.push_back( std::move( declaration ) );
	}
	return not_found;
}

Scope::list_of_sptrs<Declaration>& Scope::GetDeclarations() {
	return declarations;
}

Scope::unordered_class& Scope::GetClasses(){
	return parsed_classes;
}

Scope::omap_ptrs<ParsedFunction>& Scope::GetFunctions(){
	return parsed_functions;
}

int Scope::LocalCount() const { return local_count; }

void Scope::SetLocalCount( int count ){
	local_count = count;
}

Declaration* Scope::FindVariableDeclaration( std::wstring const & identifier ){
	for ( Scope *scope = this; scope != nullptr; scope = scope->parent ){
		auto & scope_declaration = scope->GetDeclarations();
		auto find_iter = std::find_if( std::begin( scope_declaration ), std::end( scope_declaration ),
			[ &identifier ] ( std::shared_ptr<Declaration> & decl ){ return decl->GetIdentifier() == identifier; } );
		if ( find_iter != scope_declaration.cend() ){
			return find_iter->get();
		}
	}
	return nullptr;
}

std::shared_ptr<ParsedFunction> Scope::FindFunctionDefinition( std::wstring const & function_name, unsigned int const arity )
{
	Scope *scope = this;
	do {
		auto function_iter = parsed_functions.equal_range( function_name );
		if ( function_iter.first != function_iter.second ){
			for ( auto begin_iter = function_iter.first; begin_iter != function_iter.second; ++begin_iter ){
				if ( arity == begin_iter->second->GetParameters()->Length() ){
					return begin_iter->second;
				}
			}
		}
		scope = scope->GetParentScope();
	} while ( scope != nullptr );
	return nullptr;
}

// to-do
std::shared_ptr<ParsedClass> Scope::FindClassDefinition( std::wstring const & class_name )
{
	return nullptr;
}
