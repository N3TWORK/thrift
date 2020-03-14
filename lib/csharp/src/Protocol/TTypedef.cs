namespace Thrift.Protocol 
{
	// interface for thrift typedef types ('typedef Foo Bar' => 'struct Bar : TTypedef<Foo>')
	public partial interface TTypedef<T> 
	{
		T GetValue();
		void SetValue(T value);
	}
}

