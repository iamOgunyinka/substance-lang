#pragma once

#include <memory>
#include <vector>

namespace compiler {
	// forward declarations
	class ParsedProgram;
	class Scope;

	class SemaCheck1
	{
		std::vector<std::wstring>	error_messages;
		public:
		SemaCheck1();
		~SemaCheck1() = default;

		bool Visit( compiler::ParsedProgram* program );
		void AnalyzeScope( Scope* );
		void ReportErrors();
		
		void AppendError( std::wstring const & );
		
	};

	class SemaCheck2
	{
		Scope						*current_scope;
		bool						parsingIteration;
		bool						parsingSelection;
		std::vector<std::wstring>	error_messages;
	public:
		SemaCheck2();
		void ReportErrors();

		inline bool IsParsingIteration(){ return parsingIteration; }
		inline bool IsParsingSelection(){ return parsingSelection; }
		inline Scope*  GetCurrentScope(){ return current_scope; }

		void SetParsingIteration( bool flag ){
			parsingIteration = flag;
		}
		void SetParsingSelection( bool flag ){
			parsingSelection = flag;
		}
		void SetCurrentScope( Scope *scope ){
			current_scope = scope;
		}

		bool Visit( compiler::ParsedProgram* parsed_program );
	};
}