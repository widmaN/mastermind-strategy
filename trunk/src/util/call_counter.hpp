#ifndef UTILITIES_CALL_COUNTER_HPP
#define UTILITIES_CALL_COUNTER_HPP

/// Define <code>ENABLE_CALL_COUNTER = 1</code> to enable call counter
/// support at run time. You can also use the -D compiler option.
#ifndef ENABLE_CALL_COUNTER
#define ENABLE_CALL_COUNTER 1
#endif

#include <string>
#include <map>
#include <iostream>
#include <iomanip>
#include "intrinsic.hpp"

namespace util {

#if ENABLE_CALL_COUNTER

///
/// It supports to up to (2^32-1) operations per call.
/// The total number of calls (or operations) is up to (2^64-1).
class call_counter
{
	//std::string _name; // name of the function being profiled

	// Classifies the call statistics into groups, grouped by the 
	// number of operations in a call. Calls with zero operations
	// are not recorded. Calls with one or more operations are
	// grouped like this:
	// [1,2), [2,4), [4,8), ..., [2^31,2^32)
	struct group_t
	{
		unsigned long long calls; // number of calls
		unsigned long long ops;   // number of operations
		group_t() : calls(0), ops(0) { }
	};
	static const int N = sizeof(unsigned int)*8;
	group_t _stat[N];

public:

	//call_counter(const char *name) : _name(name) { }

	//std::string name() const { return _name; }

	unsigned long long total_calls() const 
	{
		unsigned long long total = 0;
		for (int i = 0; i < N; ++i)
			total += _stat[i].calls;
		return total;
	}

	unsigned long long total_ops() const 
	{
		unsigned long long total = 0;
		for (int i = 0; i < N; ++i)
			total += _stat[i].ops;
		return total;
	}

	void add_call(unsigned int ops)
	{
		if (ops)
		{
			// The group is the location of the most significant 1.
			int pos = util::intrinsic::bit_scan_reverse(ops);
			_stat[pos].calls++;
			_stat[pos].ops += ops;
		}
	}

	friend std::ostream& operator << (std::ostream &os, const call_counter &cc)
	{
		unsigned long long ncalls = cc.total_calls();
		unsigned long long nops = cc.total_ops();

		//os << "==== Call Statistics for " << cc.name() << "() ====" << std::endl;
		os << "Total # of calls : " << ncalls << std::endl;
		os << "Total # of ops   : " << nops << std::endl;

		if (ncalls > 0)
		{
			double ops_per_call = (double)nops / ncalls;
			os << "Avg ops per call : " << ops_per_call << std::endl;
			os << "#Ops             %Calls    %Ops     #Calls         #Ops" << std::endl;
			for (int k = call_counter::N - 1; k >= 0; k--)
			{
				if (cc._stat[k].calls > 0)
				{
					os << "[" << std::setw(6) << (1<<k) << " -"
						<< std::setw(6) << ((1<<(k+1))-1) << "] "
						<< std::setw(6) << std::setprecision(2) << std::fixed
						<< (double)cc._stat[k].calls / ncalls * 100.0 << "  "
						<< std::setw(6) << std::setprecision(2) << std::fixed
						<< (double)cc._stat[k].ops / nops * 100.0 << " "
						<< std::setw(10) << cc._stat[k].calls << " "
						<< std::setw(12) << cc._stat[k].ops << std::endl;
				}
			}
		}
		return os;
	}

public:

	static call_counter& get(const std::string &name)
	{
		static std::map<std::string, call_counter> ccs;
		return ccs[name];
	}
};
#endif // ENABLE_CALL_COUNTER

#if ENABLE_CALL_COUNTER
#define REGISTER_CALL_COUNTER(id) \
	static util::call_counter& _call_counter_##id = \
	util::call_counter::get(#id);
#define UPDATE_CALL_COUNTER(id,nops) \
	(_call_counter_##id).add_call(nops)
#else
#define REGISTER_CALL_COUNTER(id)
#define UPDATE_CALL_COUNTER(id,nops)
#endif

} // namespace util

#endif // UTILITIES_CALL_COUNTER_HPP
