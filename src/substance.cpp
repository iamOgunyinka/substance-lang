/***************************************************************************
 * Language starting point
 *
 * Copyright (c) 2017 Joshua Ogunyinka
 * All rights reserved.
 */

#include <memory>
#include "parser.h"
#include "semacheck.h"

/*
#include "emitter.h"
#include "runtime.h"
*/

int main( int argc, const char* argv [] ) {
	if ( argc == 2 ) {

		using compiler::Parser;
		using compiler::ParsedProgram;
		using compiler::SemaCheck1;
		using compiler::SemaCheck2;

		std::unique_ptr<ParsedProgram> parsed_program{};
		{
			Parser parser( BytesToUnicode( argv[1] ) );
			parsed_program = parser.Parse();
		}

		if ( parsed_program ) {
			// all non-local variable declarations
			SemaCheck1 non_local_decl_sema{};
			if ( !parsed_program->Visit( non_local_decl_sema ) ){
				non_local_decl_sema.ReportErrors();
				return -1;
			}

			// to-do all local declarations
			SemaCheck2 local_decl_sema{};
			if ( !parsed_program->Visit( local_decl_sema ) ){
				local_decl_sema.ReportErrors();
				return -1;
			}

			/*
			compiler::Emitter emitter{ std::move( parsed_program ) };
			std::unique_ptr<ExecutableProgram> executable_program{ emitter.Emit() };
			if ( executable_program ) {

			runtime::Runtime runtime{ std::move( executable_program ), emitter.GetLastLabelId() };
			runtime.Run();
			return 0;
			}
			*/
			// clean up
			//compiler::Emitter::ClearInstructions();
		}
	}

	return -1;
}
