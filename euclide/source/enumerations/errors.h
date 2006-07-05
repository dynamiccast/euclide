#ifndef __EUCLIDE_ERRORS_H
#define __EUCLIDE_ERRORS_H

#include <cassert>
#undef assert

#include "euclide.h"

namespace euclide 
{

/* -------------------------------------------------------------------------- */

typedef enum
{
	NoError,
	NoSolution,

	IncorrectInputError,
	InternalLogicError,
	OutOfMemoryError,

	NumErrors

} Error;

/* -------------------------------------------------------------------------- */

void abort(Error error);
void assert(bool expression);

/* -------------------------------------------------------------------------- */

static inline
EUCLIDE_Status getStatus(Error error)
{
	if (error == NoError)
		return EUCLIDE_STATUS_OK;

	if (error == IncorrectInputError)
		return EUCLIDE_STATUS_INCORRECT_INPUT_ERROR;

	if (error == InternalLogicError)
		return EUCLIDE_STATUS_INTERNAL_LOGIC_ERROR;

	if (error == OutOfMemoryError)
		return EUCLIDE_STATUS_OUT_OF_MEMORY_ERROR;

	assert(false);
	return EUCLIDE_STATUS_UNKNOWN_ERROR;
}

/* -------------------------------------------------------------------------- */

}

#endif
