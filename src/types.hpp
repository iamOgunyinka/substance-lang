#pragma once

namespace compiler
{
	struct SemaType
	{
	private:
		enum class Types {
			BooleanType,
			CharacterType,
			IntegerType,
			FloatType,
			StringType,
			ListType,
			DictionaryType,
			LambdaType,
			FunctionType,
			UserDefinedType
		};
		SemaType() = default;

		union Value {

		};
	public:
		static SemaType* GetBoolean();
		static SemaType* GetInteger();
		static SemaType* GetFloat();
		static SemaType* GetString();
	};


	struct BooleanType : SemaType
	{

	};
}