/***************************************************************************
 * Parse tree.
 * Copyright (c) 2017 Joshua Ogunyinka
 */

#include "tree.h"

using namespace compiler;

/****************************
* ParsedFunction class
****************************/
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

void Scope::AddStatement( Statement* statement ){
	statements.push_back( statement );
}

Scope::list_of_ptrs<Statement>& Scope::GetStatements(){
	return statements;
}

bool Scope::AddDeclaration( Declaration *declaration )
{
	return declarations->AddDeclaration( declaration );
}

int Scope::LocalCount() const { return local_count; }

void Scope::SetLocalCount( int count )
{
	local_count = count;
}

//to-do
Declaration* Scope::FindDeclaration( std::wstring const & identifier )
{
	
	return nullptr;
}

bool DeclarationList::AddDeclaration( Declaration* declaration ){
	auto find_decl_iter = std::find_if( declarations.begin(), declarations.end(), [ &declaration ]( Declaration* decl ){
		return decl->GetName() == declaration->GetName();
	} );
	if ( find_decl_iter == declarations.end() ){
		declarations.push_back( std::move( declaration ) );
		return true;
	}
	if ( declaration->GetStatementType() == StatementType::FUNCTION_DECL_STMT ){
		FunctionDeclaration *func_decl = dynamic_cast< FunctionDeclaration* >( declaration );
		unsigned int const param_count = func_decl->GetParameters()->Length();
		std::wstring const & function_name = func_decl->GetName();

		DeclarationList::declaration_list_t::iterator overloaded_func_iter = std::find_if( 
			declarations.begin(), declarations.end(), [ &param_count, &function_name ] ( Declaration* decl ){
			FunctionDeclaration *func = dynamic_cast< FunctionDeclaration* >( decl );
			return decl->GetName() == function_name && func && func->GetParameters()->Length() == param_count;
		} );
		if ( overloaded_func_iter == declarations.end() ){
			declarations.push_back( std::move( declaration ) );
			return true;
		}
	}
	return false;
}

DeclarationList* Scope::GetDeclarations()
{
	return declarations;
}

Scope::Scope( Scope * p ): parent( p ), local_count( 0 ){
}

Scope::~Scope(){
	if ( declarations ){
		for ( Declaration *decl : declarations->GetDeclarations() ){
			delete decl;
			decl = nullptr;
		}
		delete declarations;
		declarations = nullptr;
	}
	for ( Statement *statement : statements ){
		if ( statement ){
			delete statement;
			statement = nullptr;
		}
	}
	statements.clear();
}

