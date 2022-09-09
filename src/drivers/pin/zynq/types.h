/*
 * \brief  Zynq-7000 pin driver types
 * \author Johannes Schlatow
 * \date   2022-09-09
 */

#ifndef _TYPES_H_
#define _TYPES_H_

/* local includes */
#include <common_types.h>

namespace Pin_driver {

	using namespace Genode;

	struct Bank;
	struct Pin_id;
}


struct Pin_driver::Bank
{
	enum Value { BANK0 = 0, BANK1, BANK2, BANK3, NUM } value;

	class Invalid : Exception { };

	static Bank from_xml(Xml_node node)
	{
		typedef String<2> Name;
		Name name = node.attribute_value("bank", Name());

		if (name == "0") return { BANK0 };
		if (name == "1") return { BANK1 };
		if (name == "2") return { BANK2 };
		if (name == "3") return { BANK3 };

		warning("unknown PIO bank name '", name, "'");
		throw Invalid();
	};
};


/**
 * Unique physical identifier of a pin
 */
struct Pin_driver::Pin_id
{
	Bank  bank;
	Index index;

	Pin_id() = delete;

	static Pin_id from_xml(Xml_node const &node)
	{
		Bank  bank  = Bank ::from_xml(node);
		Index index = Index::from_xml(node);

		bool index_out_of_bounds = false;
		if (bank.value == 1 && index.value > 21) {
			index.value = 21;
			index_out_of_bounds = true;
		}
		else if (index.value > 31) {
			index.value = 31;
			index_out_of_bounds = true;
		}

		if (index_out_of_bounds)
			error("Pin index of bank ", (unsigned)bank.value, " exceeds maximum (", (unsigned)index.value, ")");

		return { .bank  = bank,
		         .index = index };
	}

	bool operator == (Pin_id const &other) const
	{
		return other.bank.value  == bank.value
		    && other.index.value == index.value;
	}

	bool operator != (Pin_id const &other) const { return !(operator == (other)); }

	void print(Output &out) const
	{
		/* calculate absolute pin number (bank 1 has only 22 pins) */
		unsigned offset = 0;
		if (bank.value == 1)
			offset += 32;
		else if (bank.value >= 2)
			offset += 54 + 32*(bank.value-2);

		Genode::print(out, "P", index.value + offset);
	}
};

#endif /* _TYPES_H_ */
