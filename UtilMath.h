#pragma once

inline bool CheckAddOverflow(BYTE& target, BYTE& adder, BYTE& result)
{
	BYTE minimum = std::min(target, adder);
	if (result < minimum) return true;
	else return false;
}

inline bool CheckMinusOverflow(BYTE& target, BYTE& adder)
{
	if (adder > target) return true;
	else return false;
}
