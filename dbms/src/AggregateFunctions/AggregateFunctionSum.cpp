#include <AggregateFunctions/AggregateFunctionFactory.h>
#include <AggregateFunctions/AggregateFunctionSum.h>
#include <AggregateFunctions/Helpers.h>
#include <AggregateFunctions/FactoryHelpers.h>
#include "registerAggregateFunctions.h"


namespace DB
{

namespace
{

template <typename T>
struct SumSimple
{
    /// @note It uses slow Decimal128 (cause we need such a variant). sumWithOverflow is faster for Decimal32/64
    using ResultType = std::conditional_t<IsDecimalNumber<T>, Decimal128, NearestFieldType<T>>;
    using AggregateDataType = AggregateFunctionSumData<ResultType>;
    using Function = AggregateFunctionSum<T, ResultType, AggregateDataType>;
};

template <typename T>
struct SumSameType
{
    using ResultType = T;
    using AggregateDataType = AggregateFunctionSumData<ResultType>;
    using Function = AggregateFunctionSum<T, ResultType, AggregateDataType>;
};

template <typename T>
struct SumKahan
{
    using ResultType = Float64;
    using AggregateDataType = AggregateFunctionSumKahanData<ResultType>;
    using Function = AggregateFunctionSum<T, ResultType, AggregateDataType>;
};

template <typename T> using AggregateFunctionSumSimple = typename SumSimple<T>::Function;
template <typename T> using AggregateFunctionSumWithOverflow = typename SumSameType<T>::Function;
template <typename T> using AggregateFunctionSumKahan =
    std::conditional_t<IsDecimalNumber<T>, typename SumSimple<T>::Function, typename SumKahan<T>::Function>;


template <template <typename> class Function>
AggregateFunctionPtr createAggregateFunctionSum(const std::string & name, const DataTypes & argument_types, const Array & parameters)
{
    assertNoParameters(name, parameters);
    assertUnary(name, argument_types);

    AggregateFunctionPtr res;
    DataTypePtr data_type = argument_types[0];
    if (isDecimal(data_type))
        res.reset(createWithDecimalType<Function>(*data_type, *data_type, argument_types));
    else
        res.reset(createWithNumericType<Function>(*data_type, argument_types));

    if (!res)
        throw Exception("Illegal type " + argument_types[0]->getName() + " of argument for aggregate function " + name,
                        ErrorCodes::ILLEGAL_TYPE_OF_ARGUMENT);
    return res;
}

}

void registerAggregateFunctionSum(AggregateFunctionFactory & factory)
{
    factory.registerFunction("sum", createAggregateFunctionSum<AggregateFunctionSumSimple>, AggregateFunctionFactory::CaseInsensitive);
    factory.registerFunction("sumWithOverflow", createAggregateFunctionSum<AggregateFunctionSumWithOverflow>);
    factory.registerFunction("sumKahan", createAggregateFunctionSum<AggregateFunctionSumKahan>);
}

}
