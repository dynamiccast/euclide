#ifndef __EUCLIDE_ITERATOR_H
#define __EUCLIDE_ITERATOR_H

#include "../includes.h"

namespace Euclide
{

/* -------------------------------------------------------------------------- */

template <typename Enum, int NumValues, int FirstValue = 0>
struct EnumRange
{
	class EnumIterator
	{
		public :
			EnumIterator(int value) : _value(value) {}

		public :
			inline bool operator!=(const EnumIterator& iterator) const
				{ return _value != iterator._value; }

			inline void operator++()
				{ _value += 1; }

			inline Enum operator*() const
				{ return static_cast<Enum>(_value); }

		private :
			int _value;
	};

	inline EnumIterator begin() const
		{ return EnumIterator(FirstValue); }
	inline EnumIterator end() const
		{ return EnumIterator(NumValues); }
};

/* -------------------------------------------------------------------------- */

}

#endif