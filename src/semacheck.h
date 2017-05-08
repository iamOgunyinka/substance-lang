#pragma once

#include <memory>
#include <vector>

namespace compiler {
	// forward declarations
	class ParsedProgram;
	class Scope;

	class SemaCheck
	{
		std::vector<std::wstring>	error_messages;
		Scope						*current_scope;
		bool						parsingIteration;
		bool						parsingSelection;

	private:
		/*
		void Visit( std::unique_ptr<Statement> & statement );
		void Visit( std::unique_ptr<Expression> & expr );
		*/
	public:
		SemaCheck();
		~SemaCheck() = default;
		
		bool Visit( std::unique_ptr<compiler::ParsedProgram> & parsed_program );
		void ReportErrors();
		
		void AppendError( std::wstring const & );
		
		inline bool IsParsingIteration(){ return parsingIteration; }
		inline bool IsParsingSelection(){ return parsingSelection; }
		inline Scope*  GetCurrentScope(){ return current_scope; }

		void SetParsingIteration( bool flag ){
			parsingIteration = flag;
		}
		void SetParsingSelection( bool flag ){
			parsingSelection = flag;
		}
	};
}