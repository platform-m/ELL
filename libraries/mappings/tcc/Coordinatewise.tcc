// Coordinatewise.tcc

namespace mappings
{
    template <typename IndexValueIterator>
    Coordinatewise::Coordinatewise(IndexValueIterator begin, IndexValueIterator end, function<double(double, double)> func) : _func(func)
    {
        while(begin != end)
        {
            _indexValues.emplace_back(begin->GetIndex(), begin->Get());
            ++begin;
        }
    }
}