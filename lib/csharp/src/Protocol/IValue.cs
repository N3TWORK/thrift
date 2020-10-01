namespace Thrift.Protocol 
{
	// interface for thrift types wrapping a single value (e.g. typedefs) ('typedef Foo Bar' => 'struct Bar : IValue<Foo>')
	public partial interface IValue<T> 
	{
		T GetValue();
		void SetValue(T value);
	}
}

