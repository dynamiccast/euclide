#ifndef __STRINGS_H
#define __STRINGS_H

#include "includes.h"

/* -------------------------------------------------------------------------- */

class Strings
{
	public:
		typedef enum { PressAnyKey, NumTexts } Text;
		typedef enum { NoArguments, InvalidArguments, InvalidProblem, InvalidInputFile, UserInterruption, NumErrors } Error;
		typedef enum { ForsytheSymbols, GlyphSymbols, Moves, Dot, Input, Output, Score, Positions, Solution, Colon, NoSolution, UniqueSolution, OneSolution, TwoSolutions, ThreeSolutions, FourSolutions, Cooked, NumStrings } String;

	public:
		Strings();

		inline const wchar_t *operator[](Text text) const
			{ return m_texts[text]; }
		inline const wchar_t *operator[](Error error) const
			{ return m_errors[error]; }
		inline const wchar_t *operator[](String string) const
			{ return m_strings[string]; }

		inline const wchar_t *operator[](EUCLIDE_Status status) const
			{ return m_statuses[status]; }
		inline const wchar_t *operator[](EUCLIDE_Message message) const
			{ return m_messages[message]; }

	private:
		const wchar_t **m_texts;           /**< Strings for each text defined above. */
		const wchar_t **m_errors;          /**< Strings for each error defined above. */
		const wchar_t **m_strings;         /**< Strings for each string defined above. */
		const wchar_t **m_statuses;        /**< Strings for Euclide statuses. */
		const wchar_t **m_messages;        /**< Strings for Euclide messages. */
};

/* -------------------------------------------------------------------------- */

#endif
