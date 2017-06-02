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

bool Scope::AddDeclaration( Declaration *decl )
{
	if ( !declarations ){ // perhaps the first declaration, makes sense to use the filename and line number
		declarations = new DeclarationList( decl->GetFileName(), decl->GetLineNumber() );
	}
	return declarations->AddDeclaration( decl );
}

int Scope::LocalCount() const { return local_count; }

void Scope::SetLocalCount( int count )
{
	local_count = count;
}

Declaration* Scope::FindDeclaration( std::wstring const & identifier )
{
	Scope *current_scope = this;
	do {
		DeclarationList *decl_list = current_scope->GetDeclarationList();
		if ( decl_list ){
			auto declarations = decl_list->GetDeclarations();
			auto find_iter = declarations.equal_range( identifier );
			if ( find_iter.first != find_iter.second ){
				return ( find_iter.first )->second;
			}
		}
		current_scope = current_scope->GetParentScope();
	} while ( current_scope );
	return nullptr;
}

bool DeclarationList::AddDeclaration( Declaration * declaration ){
	if ( !declaration ) return false;
	if ( declaration->GetStatementType() == StatementType::VARIABLE_DECL_STMT ||
		declaration->GetStatementType() == StatementType::CLASS_DECL_STMT ){
		auto find_decl_iter = declarations.find( declaration->GetName() );
		if ( find_decl_iter == declarations.cend() ){
			declarations.insert( { declaration->GetName(), declaration } );
			return true;
		}
		return false;
	}
	else if ( declaration->GetStatementType() == StatementType::VDECL_LIST_STMT ){
		DeclarationList *decl_list = dynamic_cast< DeclarationList* >( declaration );
		for ( std::pair<std::wstring const, Declaration *> &decl : decl_list->GetDeclarations() ){
			if ( !AddDeclaration( decl.second ) ){
				return false;
			}
		}
		return true;
	}
	else if ( declaration->GetStatementType() == StatementType::FUNCTION_DECL_STMT ){
		FunctionDeclaration const *func_decl = dynamic_cast< FunctionDeclaration const * >( declaration );
		unsigned int const param_count = func_decl->GetParameters() ? func_decl->GetParameters()->Length() : 0;
		std::wstring const & function_name = func_decl->GetName();

		auto overloaded_func_range_pair = declarations.equal_range( function_name );
		for ( declaration_list_t::iterator first = overloaded_func_range_pair.first, second = overloaded_func_range_pair.second;
			first != second; ++first ){
			if ( first->second->GetStatementType() != StatementType::FUNCTION_DECL_STMT ) return false;
			auto param_expr_list = dynamic_cast< FunctionDeclaration* >( first->second )->GetParameters();
			unsigned int const local_param_count = param_expr_list ? param_expr_list->Length() : 0;
			if ( local_param_count == param_count ) return false;
		}
		declarations.insert( { function_name, declaration } );
		return true;
	}
	return false;
}

DeclarationList* Scope::GetDeclarationList()
{
	return declarations;
}

Scope::Scope( Scope * p ) : parent( p ), declarations( nullptr ), local_count( 0 ), type( ScopeType::FUNCTION_SCOPE ){
}

// to-do: properly delete declarations manually added to declarations in the first sema.
// current status: No delete taken place, thus memory leak.
Scope::~Scope(){
	for ( Statement *statement : statements ){
		if ( statement ){
			delete statement;
			statement = nullptr;
		}
	}
	statements.clear();
}

